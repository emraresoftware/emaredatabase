/*
 * ZeusDB - WAL (Write-Ahead Log)
 *
 * ACID garantisi için tüm değişiklikler önce WAL'a yazılır.
 * Crash recovery: WAL'dan okuayarak veritabanı tutarlı duruma getirilir.
 *
 * WAL Formatı:
 *   [WALRecord header][data bytes][WALRecord header][data bytes]...
 *
 * Her segment 16MB. Dolu segment arşivlenir, yenisi açılır.
 * Checkpoint: Dirty page'ler diske yazılır, eski WAL silinebilir.
 */

#include "../include/zeusdb.h"

/* ============================================================
 * WAL DOSYA YÖNETİMİ
 * ============================================================ */

static void wal_segment_path(const char *dirpath, uint32_t segment, char *path, size_t len) {
    snprintf(path, len, "%s/wal_%08u.log", dirpath, segment);
}

static ZeusStatus wal_open_segment(WALManager *wal) {
    char path[1024];
    wal_segment_path(wal->dirpath, wal->current_segment, path, sizeof(path));

    wal->fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (wal->fd < 0) {
        zeus_log("ERROR", "WAL segment açılamadı: %s (%s)", path, strerror(errno));
        return ZEUS_ERROR_IO;
    }

    zeus_log("INFO", "WAL segment açıldı: %s", path);
    return ZEUS_OK;
}

static ZeusStatus wal_rotate_segment(WALManager *wal) {
    if (wal->fd >= 0) {
        fsync(wal->fd);
        close(wal->fd);
    }

    wal->current_segment++;
    return wal_open_segment(wal);
}

/* ============================================================
 * WAL AÇMA / KAPAMA
 * ============================================================ */

ZeusStatus wal_open(WALManager **wal, const char *dirpath) {
    if (!wal || !dirpath) return ZEUS_ERROR;

    WALManager *w = calloc(1, sizeof(WALManager));
    if (!w) return ZEUS_ERROR_NOMEM;

    strncpy(w->dirpath, dirpath, sizeof(w->dirpath) - 1);

    /* WAL dizinini oluştur */
    char wal_dir[1024];
    snprintf(wal_dir, sizeof(wal_dir), "%s", dirpath);
    mkdir(wal_dir, 0755);

    w->current_lsn = 1;
    w->flushed_lsn = 0;
    w->checkpoint_lsn = 0;
    w->current_segment = 0;

    /* Buffer */
    w->buffer_size = WAL_SEGMENT_SIZE;
    w->buffer = malloc(w->buffer_size);
    if (!w->buffer) {
        free(w);
        return ZEUS_ERROR_NOMEM;
    }
    w->buffer_pos = 0;

    pthread_mutex_init(&w->lock, NULL);

    /* İlk segmenti aç */
    ZeusStatus rc = wal_open_segment(w);
    if (rc != ZEUS_OK) {
        free(w->buffer);
        free(w);
        return rc;
    }

    /* Mevcut LSN'yi dosyadan oku */
    struct stat st;
    if (fstat(w->fd, &st) == 0 && st.st_size > 0) {
        /* Dosyanın sonuna git, son kaydın LSN'ini al */
        off_t pos = 0;
        WALRecord rec;
        uint64_t max_lsn = 0;

        lseek(w->fd, 0, SEEK_SET);
        while (pread(w->fd, &rec, sizeof(WALRecord), pos) == sizeof(WALRecord)) {
            if (rec.lsn > max_lsn) max_lsn = rec.lsn;
            pos += sizeof(WALRecord) + rec.data_len;
        }

        if (max_lsn > 0) {
            w->current_lsn = max_lsn + 1;
            w->flushed_lsn = max_lsn;
        }
    }

    zeus_log("INFO", "WAL başlatıldı: %s (LSN: %llu)", dirpath, (unsigned long long)w->current_lsn);
    *wal = w;
    return ZEUS_OK;
}

ZeusStatus wal_close(WALManager *wal) {
    if (!wal) return ZEUS_ERROR;

    wal_flush(wal);

    if (wal->fd >= 0) {
        fsync(wal->fd);
        close(wal->fd);
    }

    if (wal->buffer) free(wal->buffer);
    pthread_mutex_destroy(&wal->lock);

    zeus_log("INFO", "WAL kapatıldı (son LSN: %llu)", (unsigned long long)wal->current_lsn);
    free(wal);
    return ZEUS_OK;
}

/* ============================================================
 * WAL YAZMA
 * ============================================================ */

ZeusStatus wal_write(WALManager *wal, const WALRecord *record, const uint8_t *data) {
    if (!wal || !record) return ZEUS_ERROR;

    pthread_mutex_lock(&wal->lock);

    /* Record'u hazırla */
    WALRecord rec = *record;
    rec.lsn = wal->current_lsn++;
    rec.timestamp = (uint64_t)time(NULL);

    /* Checksum hesapla */
    uint32_t cksum = pager_checksum((const uint8_t *)&rec, sizeof(WALRecord) - sizeof(uint32_t));
    if (data && rec.data_len > 0) {
        cksum ^= pager_checksum(data, rec.data_len);
    }
    rec.checksum = cksum;

    /* Buffer'a yaz */
    uint32_t total_size = sizeof(WALRecord) + rec.data_len;
    if (wal->buffer_pos + total_size > wal->buffer_size) {
        /* Buffer dolu - flush et */
        wal_flush(wal);
    }

    memcpy(wal->buffer + wal->buffer_pos, &rec, sizeof(WALRecord));
    wal->buffer_pos += sizeof(WALRecord);

    if (data && rec.data_len > 0) {
        memcpy(wal->buffer + wal->buffer_pos, data, rec.data_len);
        wal->buffer_pos += rec.data_len;
    }

    /* Segment boyutu kontrolü */
    struct stat st;
    if (fstat(wal->fd, &st) == 0 && (uint32_t)st.st_size + wal->buffer_pos > WAL_SEGMENT_SIZE) {
        wal_flush(wal);
        wal_rotate_segment(wal);
    }

    pthread_mutex_unlock(&wal->lock);
    return ZEUS_OK;
}

/* ============================================================
 * WAL FLUSH
 * ============================================================ */

ZeusStatus wal_flush(WALManager *wal) {
    if (!wal || wal->buffer_pos == 0) return ZEUS_OK;

    /* Buffer'ı diske yaz */
    ssize_t written = write(wal->fd, wal->buffer, wal->buffer_pos);
    if (written != (ssize_t)wal->buffer_pos) {
        zeus_log("ERROR", "WAL flush hatası: yazılan %zd / %u", written, wal->buffer_pos);
        return ZEUS_ERROR_IO;
    }

    fsync(wal->fd);
    wal->flushed_lsn = wal->current_lsn - 1;
    wal->buffer_pos = 0;

    return ZEUS_OK;
}

/* ============================================================
 * CHECKPOINT
 * ============================================================ */

ZeusStatus wal_checkpoint(WALManager *wal, Pager *pager) {
    if (!wal || !pager) return ZEUS_ERROR;

    pthread_mutex_lock(&wal->lock);

    zeus_log("INFO", "Checkpoint başlatılıyor (LSN: %llu -> %llu)",
             (unsigned long long)wal->checkpoint_lsn,
             (unsigned long long)wal->current_lsn);

    /* Tüm dirty page'leri diske yaz */
    ZeusStatus rc = pager_flush(pager);
    if (rc != ZEUS_OK) {
        pthread_mutex_unlock(&wal->lock);
        return rc;
    }

    /* WAL flush */
    wal_flush(wal);

    /* Checkpoint kaydı yaz */
    WALRecord checkpoint_rec = {0};
    checkpoint_rec.type = WAL_CHECKPOINT;
    checkpoint_rec.data_len = 0;

    /* Doğrudan diske yaz */
    checkpoint_rec.lsn = wal->current_lsn++;
    checkpoint_rec.timestamp = (uint64_t)time(NULL);
    checkpoint_rec.checksum = pager_checksum((const uint8_t *)&checkpoint_rec,
                                              sizeof(WALRecord) - sizeof(uint32_t));

    write(wal->fd, &checkpoint_rec, sizeof(WALRecord));
    fsync(wal->fd);

    wal->checkpoint_lsn = checkpoint_rec.lsn;

    /* Eski segmentleri temizle */
    if (wal->current_segment > 1) {
        for (uint32_t i = 0; i < wal->current_segment - 1; i++) {
            char path[1024];
            wal_segment_path(wal->dirpath, i, path, sizeof(path));
            if (unlink(path) == 0) {
                zeus_log("DEBUG", "Eski WAL segment silindi: %s", path);
            }
        }
    }

    zeus_log("INFO", "Checkpoint tamamlandı (LSN: %llu)",
             (unsigned long long)wal->checkpoint_lsn);

    pthread_mutex_unlock(&wal->lock);
    return ZEUS_OK;
}

/* ============================================================
 * CRASH RECOVERY
 * ============================================================ */

ZeusStatus wal_recover(WALManager *wal, Database *db) {
    if (!wal || !db) return ZEUS_ERROR;

    zeus_log("INFO", "WAL recovery başlatılıyor...");

    /* WAL segment'lerini sırayla oku */
    uint32_t recovered = 0;
    uint32_t seg = 0;

    while (seg <= wal->current_segment) {
        char path[1024];
        wal_segment_path(wal->dirpath, seg, path, sizeof(path));

        int fd = open(path, O_RDONLY);
        if (fd < 0) { seg++; continue; }

        off_t pos = 0;
        WALRecord rec;

        while (read(fd, &rec, sizeof(WALRecord)) == sizeof(WALRecord)) {
            /* Sadece checkpoint sonrası kayıtları işle */
            if (rec.lsn <= wal->checkpoint_lsn) {
                if (rec.data_len > 0) lseek(fd, rec.data_len, SEEK_CUR);
                pos += sizeof(WALRecord) + rec.data_len;
                continue;
            }

            /* Recovery record'unu oku */
            uint8_t *data = NULL;
            if (rec.data_len > 0) {
                data = malloc(rec.data_len);
                if (data) {
                    if (read(fd, data, rec.data_len) != (ssize_t)rec.data_len) {
                        free(data);
                        break;
                    }
                }
            }

            /* Checksum doğrula */
            uint32_t expected = rec.checksum;
            rec.checksum = 0;
            uint32_t actual = pager_checksum((const uint8_t *)&rec,
                                              sizeof(WALRecord) - sizeof(uint32_t));
            if (data && rec.data_len > 0) {
                actual ^= pager_checksum(data, rec.data_len);
            }

            if (expected != actual) {
                zeus_log("WARN", "WAL checksum uyumsuzluğu: LSN %llu",
                         (unsigned long long)rec.lsn);
                free(data);
                break;
            }

            /* Record'u uygula */
            switch (rec.type) {
                case WAL_INSERT:
                case WAL_UPDATE:
                case WAL_DELETE:
                    /* Data page'i güncelle */
                    if (rec.page_num > 0 && data) {
                        Page page;
                        if (pager_read_page(db->pager, rec.page_num, &page) == ZEUS_OK) {
                            memcpy(page.data, data,
                                   rec.data_len < sizeof(page.data) ? rec.data_len : sizeof(page.data));
                            pager_write_page(db->pager, rec.page_num, &page);
                        }
                    }
                    recovered++;
                    break;

                case WAL_CHECKPOINT:
                    /* Checkpoint kaydı - skip */
                    break;

                case WAL_TXN_COMMIT:
                    zeus_log("DEBUG", "Recovery: TXN %llu commit",
                             (unsigned long long)rec.txn_id);
                    break;

                case WAL_TXN_ROLLBACK:
                    zeus_log("DEBUG", "Recovery: TXN %llu rollback",
                             (unsigned long long)rec.txn_id);
                    break;

                default:
                    break;
            }

            free(data);
            pos += sizeof(WALRecord) + rec.data_len;
        }

        close(fd);
        seg++;
    }

    if (recovered > 0) {
        pager_flush(db->pager);
        zeus_log("INFO", "WAL recovery tamamlandı: %u kayıt uygulandı", recovered);
    } else {
        zeus_log("INFO", "WAL recovery: Uygulanacak kayıt yok");
    }

    return ZEUS_OK;
}
