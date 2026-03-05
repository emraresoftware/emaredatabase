/*
 * ZeusDB - Pager (Page-based Disk I/O Manager)
 * 
 * Veritabanı dosyasını sabit boyutlu sayfalar halinde yönetir.
 * Her sayfa 4096 byte. Basit in-memory cache ile.
 */

#include "../include/zeusdb.h"

/* ============================================================
 * YARDIMCI FONKSİYONLAR
 * ============================================================ */

uint32_t pager_checksum(const uint8_t *data, uint32_t len) {
    /* FNV-1a hash - hızlı ve iyi dağılımlı */
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

/* ============================================================
 * PAGER AÇMA / KAPAMA
 * ============================================================ */

ZeusStatus pager_open(Pager **pager, const char *filepath) {
    if (!pager || !filepath) return ZEUS_ERROR;

    Pager *p = calloc(1, sizeof(Pager));
    if (!p) return ZEUS_ERROR_NOMEM;

    strncpy(p->filepath, filepath, sizeof(p->filepath) - 1);

    /* Dosyayı aç/oluştur */
    p->fd = open(filepath, O_RDWR | O_CREAT, 0644);
    if (p->fd < 0) {
        zeus_log("ERROR", "Dosya açılamadı: %s (%s)", filepath, strerror(errno));
        free(p);
        return ZEUS_ERROR_IO;
    }

    /* Dosya boyutunu al */
    struct stat st;
    if (fstat(p->fd, &st) < 0) {
        close(p->fd);
        free(p);
        return ZEUS_ERROR_IO;
    }

    p->file_size = (uint32_t)st.st_size;
    p->num_pages = p->file_size / PAGE_SIZE;

    /* Cache'i sıfırla */
    memset(p->page_cache, 0, sizeof(p->page_cache));
    memset(p->dirty, 0, sizeof(p->dirty));

    /* RW lock */
    pthread_rwlock_init(&p->lock, NULL);

    /* Eğer yeni dosyaysa, meta sayfa oluştur */
    if (p->num_pages == 0) {
        Page meta;
        memset(&meta, 0, sizeof(Page));
        meta.header.page_num = 0;
        meta.header.type = PAGE_TYPE_META;
        meta.header.num_cells = 0;
        meta.header.free_space = PAGE_SIZE - sizeof(PageHeader);

        /* Meta sayfaya veritabanı bilgilerini yaz */
        const char *magic = "ZEUS";
        memcpy(meta.data, magic, 4);
        uint32_t version = (ZEUS_VERSION_MAJOR << 16) | (ZEUS_VERSION_MINOR << 8) | ZEUS_VERSION_PATCH;
        memcpy(meta.data + 4, &version, 4);
        uint32_t page_size = PAGE_SIZE;
        memcpy(meta.data + 8, &page_size, 4);

        meta.header.checksum = pager_checksum(meta.data, PAGE_SIZE - sizeof(PageHeader));

        if (write(p->fd, &meta, PAGE_SIZE) != PAGE_SIZE) {
            close(p->fd);
            free(p);
            return ZEUS_ERROR_IO;
        }

        p->num_pages = 1;
        p->file_size = PAGE_SIZE;
    } else {
        /* Mevcut dosyayı doğrula */
        Page meta;
        if (pread(p->fd, &meta, PAGE_SIZE, 0) != PAGE_SIZE) {
            close(p->fd);
            free(p);
            return ZEUS_ERROR_IO;
        }

        if (memcmp(meta.data, "ZEUS", 4) != 0) {
            zeus_log("ERROR", "Geçersiz veritabanı dosyası: %s", filepath);
            close(p->fd);
            free(p);
            return ZEUS_ERROR_CORRUPT;
        }
    }

    zeus_log("INFO", "Pager açıldı: %s (%u sayfa)", filepath, p->num_pages);
    *pager = p;
    return ZEUS_OK;
}

ZeusStatus pager_close(Pager *pager) {
    if (!pager) return ZEUS_ERROR;

    /* Tüm dirty sayfaları diske yaz */
    pager_flush(pager);

    /* Cache'deki sayfaları serbest bırak */
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (pager->page_cache[i]) {
            free(pager->page_cache[i]);
            pager->page_cache[i] = NULL;
        }
    }

    pthread_rwlock_destroy(&pager->lock);

    if (pager->fd >= 0) {
        fsync(pager->fd);
        close(pager->fd);
    }

    zeus_log("INFO", "Pager kapatıldı: %s", pager->filepath);
    free(pager);
    return ZEUS_OK;
}

/* ============================================================
 * SAYFA OKUMA / YAZMA
 * ============================================================ */

ZeusStatus pager_read_page(Pager *pager, uint32_t page_num, Page *page) {
    if (!pager || !page) return ZEUS_ERROR;
    if (page_num >= pager->num_pages) return ZEUS_ERROR_NOT_FOUND;

    pthread_rwlock_rdlock(&pager->lock);

    /* Önce cache'e bak */
    if (pager->page_cache[page_num]) {
        memcpy(page, pager->page_cache[page_num], sizeof(Page));
        pthread_rwlock_unlock(&pager->lock);
        return ZEUS_OK;
    }

    pthread_rwlock_unlock(&pager->lock);
    pthread_rwlock_wrlock(&pager->lock);

    /* Double-check (başka thread cache'e eklemiş olabilir) */
    if (pager->page_cache[page_num]) {
        memcpy(page, pager->page_cache[page_num], sizeof(Page));
        pthread_rwlock_unlock(&pager->lock);
        return ZEUS_OK;
    }

    /* Diskten oku */
    off_t offset = (off_t)page_num * PAGE_SIZE;
    ssize_t bytes_read = pread(pager->fd, page, PAGE_SIZE, offset);
    if (bytes_read != PAGE_SIZE) {
        zeus_log("ERROR", "Sayfa okunamadı: %u (okunan: %zd)", page_num, bytes_read);
        pthread_rwlock_unlock(&pager->lock);
        return ZEUS_ERROR_IO;
    }

    /* Checksum doğrula */
    uint32_t expected = page->header.checksum;
    uint32_t actual = pager_checksum(page->data, PAGE_SIZE - sizeof(PageHeader));
    if (expected != 0 && expected != actual) {
        zeus_log("WARN", "Checksum uyumsuzluğu: sayfa %u (beklenen: %u, bulunan: %u)",
                 page_num, expected, actual);
        /* Uyarı ver ama devam et - ilk oluşturulan sayfalarda 0 olabilir */
    }

    /* Cache'e ekle */
    pager->page_cache[page_num] = malloc(sizeof(Page));
    if (pager->page_cache[page_num]) {
        memcpy(pager->page_cache[page_num], page, sizeof(Page));
    }

    pthread_rwlock_unlock(&pager->lock);
    return ZEUS_OK;
}

ZeusStatus pager_write_page(Pager *pager, uint32_t page_num, const Page *page) {
    if (!pager || !page) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&pager->lock);

    /* Cache'i güncelle */
    if (!pager->page_cache[page_num]) {
        pager->page_cache[page_num] = malloc(sizeof(Page));
        if (!pager->page_cache[page_num]) {
            pthread_rwlock_unlock(&pager->lock);
            return ZEUS_ERROR_NOMEM;
        }
    }

    memcpy(pager->page_cache[page_num], page, sizeof(Page));
    pager->dirty[page_num] = true;

    /* num_pages güncelle */
    if (page_num >= pager->num_pages) {
        pager->num_pages = page_num + 1;
    }

    pthread_rwlock_unlock(&pager->lock);
    return ZEUS_OK;
}

ZeusStatus pager_allocate_page(Pager *pager, uint32_t *page_num) {
    if (!pager || !page_num) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&pager->lock);

    *page_num = pager->num_pages;

    /* Yeni sayfa oluştur */
    Page new_page;
    memset(&new_page, 0, sizeof(Page));
    new_page.header.page_num = *page_num;
    new_page.header.type = PAGE_TYPE_FREE;
    new_page.header.free_space = PAGE_SIZE - sizeof(PageHeader);

    /* Dosyayı genişlet */
    off_t offset = (off_t)(*page_num) * PAGE_SIZE;
    if (pwrite(pager->fd, &new_page, PAGE_SIZE, offset) != PAGE_SIZE) {
        pthread_rwlock_unlock(&pager->lock);
        return ZEUS_ERROR_IO;
    }

    pager->num_pages++;
    pager->file_size = pager->num_pages * PAGE_SIZE;

    /* Cache'e ekle */
    pager->page_cache[*page_num] = malloc(sizeof(Page));
    if (pager->page_cache[*page_num]) {
        memcpy(pager->page_cache[*page_num], &new_page, sizeof(Page));
    }

    pthread_rwlock_unlock(&pager->lock);

    zeus_log("DEBUG", "Yeni sayfa tahsis edildi: %u (toplam: %u)", *page_num, pager->num_pages);
    return ZEUS_OK;
}

ZeusStatus pager_free_page(Pager *pager, uint32_t page_num) {
    if (!pager || page_num >= pager->num_pages) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&pager->lock);

    /* Sayfayı FREE olarak işaretle */
    if (pager->page_cache[page_num]) {
        pager->page_cache[page_num]->header.type = PAGE_TYPE_FREE;
        memset(pager->page_cache[page_num]->data, 0, PAGE_SIZE - sizeof(PageHeader));
        pager->dirty[page_num] = true;
    }

    pthread_rwlock_unlock(&pager->lock);
    return ZEUS_OK;
}

ZeusStatus pager_flush(Pager *pager) {
    if (!pager) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&pager->lock);

    uint32_t flushed = 0;
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->dirty[i] && pager->page_cache[i]) {
            /* Checksum hesapla */
            pager->page_cache[i]->header.checksum =
                pager_checksum(pager->page_cache[i]->data, PAGE_SIZE - sizeof(PageHeader));

            off_t offset = (off_t)i * PAGE_SIZE;
            if (pwrite(pager->fd, pager->page_cache[i], PAGE_SIZE, offset) != PAGE_SIZE) {
                zeus_log("ERROR", "Sayfa yazılamadı: %u", i);
                pthread_rwlock_unlock(&pager->lock);
                return ZEUS_ERROR_IO;
            }
            pager->dirty[i] = false;
            flushed++;
        }
    }

    if (flushed > 0) {
        fsync(pager->fd);
        zeus_log("DEBUG", "%u sayfa diske yazıldı", flushed);
    }

    pthread_rwlock_unlock(&pager->lock);
    return ZEUS_OK;
}
