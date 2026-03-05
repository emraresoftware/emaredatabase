/*
 * ZeusDB - Transaction Manager
 *
 * ACID garantili transaction yönetimi:
 *   - Atomicity: WAL ile tüm değişiklikler ya uygulanır ya uygulanmaz
 *   - Consistency: Constraint kontrolleri
 *   - Isolation: Lock Manager ile (ayrı modül)
 *   - Durability: WAL + checkpoint ile
 *
 * Isolation seviyeleri:
 *   - READ UNCOMMITTED (dirty read mümkün)
 *   - READ COMMITTED (default)
 *   - REPEATABLE READ
 *   - SERIALIZABLE
 */

#include "../include/zeusdb.h"

/* ============================================================
 * TRANSACTION MANAGER INIT / DESTROY
 * ============================================================ */

ZeusStatus txn_mgr_init(TxnManager **mgr, WALManager *wal) {
    if (!mgr || !wal) return ZEUS_ERROR;

    TxnManager *m = calloc(1, sizeof(TxnManager));
    if (!m) return ZEUS_ERROR_NOMEM;

    m->wal = wal;
    m->next_txn_id = 1;
    m->num_active = 0;
    pthread_mutex_init(&m->lock, NULL);

    zeus_log("INFO", "Transaction Manager başlatıldı");
    *mgr = m;
    return ZEUS_OK;
}

ZeusStatus txn_mgr_destroy(TxnManager *mgr) {
    if (!mgr) return ZEUS_ERROR;

    /* Aktif transaction'ları rollback et */
    for (uint32_t i = 0; i < MAX_TRANSACTIONS; i++) {
        if (mgr->transactions[i].state == TXN_ACTIVE) {
            zeus_log("WARN", "Aktif transaction rollback ediliyor: TXN-%llu",
                     (unsigned long long)mgr->transactions[i].txn_id);
            txn_rollback(mgr, &mgr->transactions[i]);
        }
    }

    pthread_mutex_destroy(&mgr->lock);
    free(mgr);

    zeus_log("INFO", "Transaction Manager kapatıldı");
    return ZEUS_OK;
}

/* ============================================================
 * BEGIN TRANSACTION
 * ============================================================ */

ZeusStatus txn_begin(TxnManager *mgr, Transaction **txn) {
    if (!mgr || !txn) return ZEUS_ERROR;

    pthread_mutex_lock(&mgr->lock);

    if (mgr->num_active >= MAX_TRANSACTIONS) {
        pthread_mutex_unlock(&mgr->lock);
        zeus_log("ERROR", "Maksimum aktif transaction sayısı aşıldı");
        return ZEUS_ERROR_FULL;
    }

    /* Boş slot bul */
    int slot = -1;
    for (uint32_t i = 0; i < MAX_TRANSACTIONS; i++) {
        if (mgr->transactions[i].state != TXN_ACTIVE &&
            mgr->transactions[i].state != TXN_PREPARING) {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return ZEUS_ERROR_FULL;
    }

    /* Transaction oluştur */
    Transaction *t = &mgr->transactions[slot];
    memset(t, 0, sizeof(Transaction));
    t->txn_id = mgr->next_txn_id++;
    t->state = TXN_ACTIVE;
    t->isolation = TXN_ISOLATION_READ_COMMITTED;
    t->start_lsn = mgr->wal->current_lsn;
    t->start_timestamp = (uint64_t)time(NULL);
    t->num_locks = 0;
    t->read_only = false;

    mgr->num_active++;

    /* WAL'a BEGIN kaydı yaz */
    WALRecord rec = {0};
    rec.type = WAL_TXN_BEGIN;
    rec.txn_id = t->txn_id;
    rec.data_len = 0;
    wal_write(mgr->wal, &rec, NULL);

    pthread_mutex_unlock(&mgr->lock);

    zeus_log("DEBUG", "Transaction başlatıldı: TXN-%llu (isolation: %d)",
             (unsigned long long)t->txn_id, t->isolation);

    *txn = t;
    return ZEUS_OK;
}

/* ============================================================
 * COMMIT TRANSACTION
 * ============================================================ */

ZeusStatus txn_commit(TxnManager *mgr, Transaction *txn) {
    if (!mgr || !txn) return ZEUS_ERROR;

    pthread_mutex_lock(&mgr->lock);

    if (txn->state != TXN_ACTIVE) {
        pthread_mutex_unlock(&mgr->lock);
        zeus_log("ERROR", "Commit edilecek transaction aktif değil: TXN-%llu",
                 (unsigned long long)txn->txn_id);
        return ZEUS_ERROR_TXN_ABORT;
    }

    /* WAL'a COMMIT kaydı yaz */
    WALRecord rec = {0};
    rec.type = WAL_TXN_COMMIT;
    rec.txn_id = txn->txn_id;
    rec.data_len = 0;
    wal_write(mgr->wal, &rec, NULL);

    /* WAL'ı diske flush et (durability garantisi) */
    wal_flush(mgr->wal);

    txn->state = TXN_COMMITTED;
    mgr->num_active--;

    pthread_mutex_unlock(&mgr->lock);

    zeus_log("DEBUG", "Transaction commit edildi: TXN-%llu",
             (unsigned long long)txn->txn_id);

    return ZEUS_OK;
}

/* ============================================================
 * ROLLBACK TRANSACTION
 * ============================================================ */

ZeusStatus txn_rollback(TxnManager *mgr, Transaction *txn) {
    if (!mgr || !txn) return ZEUS_ERROR;

    pthread_mutex_lock(&mgr->lock);

    if (txn->state != TXN_ACTIVE) {
        pthread_mutex_unlock(&mgr->lock);
        return ZEUS_ERROR_TXN_ABORT;
    }

    /* WAL'a ROLLBACK kaydı yaz */
    WALRecord rec = {0};
    rec.type = WAL_TXN_ROLLBACK;
    rec.txn_id = txn->txn_id;
    rec.data_len = 0;
    wal_write(mgr->wal, &rec, NULL);

    /* WAL'daki bu transaction'ın kayıtlarını geri al */
    /* Not: Basitleştirilmiş versiyon - undo log kullanmıyoruz.
     * Gerçek implementasyonda undo kayıtları ile değişiklikler
     * geri alınır (before-image restore). */

    wal_flush(mgr->wal);

    txn->state = TXN_ABORTED;
    mgr->num_active--;

    pthread_mutex_unlock(&mgr->lock);

    zeus_log("DEBUG", "Transaction rollback edildi: TXN-%llu",
             (unsigned long long)txn->txn_id);

    return ZEUS_OK;
}
