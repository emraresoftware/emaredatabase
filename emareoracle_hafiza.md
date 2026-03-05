# 🔱 EMAREORACLE — ZeusDB Proje Hafızası

> 🔗 **Ortak Hafıza:** [`EMARE_ORTAK_HAFIZA.md`](/Users/emre/Desktop/Emare/EMARE_ORTAK_HAFIZA.md) — Tüm Emare ekosistemi, sunucu bilgileri, standartlar ve proje envanteri için bak.


> **Bu dosya, ZeusDB veritabanı motorunun tüm detaylarını içerir.**  
> **Proje sahibi:** Emre  
> **Amaç:** Oracle'dan daha iyi, C dilinde yazılmış tam kapsamlı ilişkisel veritabanı motoru  
> **Son güncelleme:** 3 Mart 2026  

---

## 📌 NEREDE KALDIK?

### Tamamlanan Modüller (5/10)
| # | Modül | Dosya(lar) | Durum |
|---|-------|-----------|--------|
| 1 | Proje yapısı & Ana Header | `include/zeusdb.h` | ✅ Tamamlandı |
| 2 | Storage Engine (Pager + B+Tree) | `src/pager.c`, `src/btree.c` | ✅ Tamamlandı |
| 3 | SQL Tokenizer & Parser | `src/tokenizer.c`, `src/parser.c` | ✅ Tamamlandı |
| 4 | Query Executor & Table Manager | `src/table.c`, `src/executor.c` | ✅ Tamamlandı |
| 5 | WAL & Transaction Manager | `src/wal.c`, `src/txn.c` | ✅ Tamamlandı |

### Yapılacak Modüller (5/10)
| # | Modül | Dosya(lar) | Durum |
|---|-------|-----------|--------|
| 6 | Concurrency — Lock Manager | `src/lock.c` (yazılacak) | ⏳ Sıradaki |
| 7 | Replication Manager | `src/replication.c` (yazılacak) | ⏳ Bekliyor |
| 8 | CLI & Client Arayüzü | `src/cli.c`, `src/server.c`, `src/main.c` (yazılacak) | ⏳ Bekliyor |
| 9 | Makefile & Build Sistemi | `Makefile` (yazılacak) | ⏳ Bekliyor |
| 10 | Test & Doğrulama | `tests/` (yazılacak) | ⏳ Bekliyor |

### Sıradaki Adım
**Lock Manager** yazılacak → Deadlock detection, shared/exclusive lock, row-level locking.

---

## 🏗️ PROJE YAPISI

```
/Users/emre/Desktop/oracle/
├── include/
│   └── zeusdb.h          (791 satır) — Ana header, tüm tipler ve prototipler
├── src/
│   ├── pager.c           (301 satır) — Sayfa tabanlı disk I/O
│   ├── btree.c           (565 satır) — B+Tree index yapısı
│   ├── tokenizer.c       (340 satır) — SQL tokenizer
│   ├── parser.c          (843 satır) — SQL parser (recursive descent)
│   ├── table.c           (732 satır) — Tablo yönetimi, row serialize, utility
│   ├── executor.c        (880 satır) — Sorgu çalıştırma motoru
│   ├── wal.c             (368 satır) — Write-Ahead Log
│   └── txn.c             (196 satır) — Transaction yönetimi
├── tests/                 (boş — yazılacak)
├── data/                  (boş — runtime veri dizini)
└── emareoracle_hafiza.md  (bu dosya)

TOPLAM: 5,016 satır C kodu (şu ana kadar)
```

---

## 🧠 ZeusDB NEDİR?

**ZeusDB**, C dilinde sıfırdan yazılmış, disk tabanlı, ilişkisel bir veritabanı motorudur.

### Temel Özellikler
- **Dil:** Pure C (C11)
- **Sayfa boyutu:** 4096 byte (4KB)
- **Maks veritabanı boyutu:** 4GB (1,048,576 sayfa)
- **Index yapısı:** B+Tree (order 128)
- **SQL desteği:** SELECT, INSERT, UPDATE, DELETE, CREATE TABLE, DROP TABLE
- **ACID garantisi:** WAL (Write-Ahead Log) ile
- **Concurrency:** pthread RW-lock tabanlı (Lock Manager henüz yazılmadı)
- **Replication:** Primary-Replica mimarisi (henüz yazılmadı)
- **Bağlantı:** TCP socket server + CLI (henüz yazılmadı)

---

## 📐 MİMARİ DETAYLAR

### 1. Sabitler ve Limitler

| Sabit | Değer | Açıklama |
|-------|-------|----------|
| `PAGE_SIZE` | 4096 | Sayfa boyutu (byte) |
| `MAX_PAGES` | 1,048,576 | Maks sayfa sayısı (4GB) |
| `PAGE_HEADER_SIZE` | 64 | Sayfa başlığı (byte) |
| `MAX_ROW_SIZE` | ~4016 | Maks satır boyutu |
| `BTREE_ORDER` | 128 | B+Tree dallanma faktörü |
| `BTREE_MAX_KEYS` | 127 | Düğüm başına maks anahtar |
| `MAX_COLUMNS` | 64 | Tablo başına maks sütun |
| `MAX_COLUMN_NAME` | 64 | Sütun adı maks uzunluk |
| `MAX_TABLE_NAME` | 128 | Tablo adı maks uzunluk |
| `MAX_TABLES` | 256 | Maks tablo sayısı |
| `MAX_VARCHAR_LEN` | 4096 | VARCHAR maks uzunluk |
| `MAX_SQL_LENGTH` | 65,536 | SQL sorgu maks uzunluk |
| `MAX_TOKENS` | 4096 | Maks token sayısı |
| `MAX_WHERE_CONDITIONS` | 32 | WHERE'da maks koşul |
| `MAX_ORDER_BY` | 8 | ORDER BY maks sütun |
| `MAX_TRANSACTIONS` | 1024 | Eşzamanlı maks transaction |
| `WAL_SEGMENT_SIZE` | 16MB | WAL segment boyutu |
| `MAX_WAL_SEGMENTS` | 256 | Maks WAL segment sayısı |
| `CHECKPOINT_INTERVAL` | 1000 | Her 1000 txn'da 1 checkpoint |
| `MAX_CONNECTIONS` | 256 | Maks eşzamanlı bağlantı |
| `MAX_LOCKS` | 4096 | Maks kilit sayısı |
| `LOCK_TIMEOUT_MS` | 30,000 | Kilit zaman aşımı (30s) |
| `THREAD_POOL_SIZE` | 16 | Thread havuzu boyutu |
| `MAX_REPLICAS` | 8 | Maks replika sayısı |

### 2. Hata Kodları (ZeusStatus)

| Kod | Değer | Açıklama |
|-----|-------|----------|
| `ZEUS_OK` | 0 | Başarılı |
| `ZEUS_ERROR` | -1 | Genel hata |
| `ZEUS_ERROR_IO` | -2 | Disk I/O hatası |
| `ZEUS_ERROR_CORRUPT` | -3 | Bozuk veri |
| `ZEUS_ERROR_FULL` | -4 | Veritabanı dolu |
| `ZEUS_ERROR_NOT_FOUND` | -5 | Bulunamadı |
| `ZEUS_ERROR_EXISTS` | -6 | Zaten mevcut |
| `ZEUS_ERROR_CONSTRAINT` | -7 | Kısıtlama ihlali |
| `ZEUS_ERROR_TYPE` | -8 | Tip uyumsuzluğu |
| `ZEUS_ERROR_SYNTAX` | -9 | SQL sözdizimi hatası |
| `ZEUS_ERROR_INTERNAL` | -10 | Dahili hata |
| `ZEUS_ERROR_TXN_ABORT` | -11 | Transaction iptal |
| `ZEUS_ERROR_DEADLOCK` | -12 | Deadlock tespit edildi |
| `ZEUS_ERROR_TIMEOUT` | -13 | Zaman aşımı |
| `ZEUS_ERROR_READONLY` | -14 | Salt okunur |
| `ZEUS_ERROR_NOMEM` | -15 | Bellek yetersiz |
| `ZEUS_ERROR_BUSY` | -16 | Meşgul |
| `ZEUS_ERROR_OVERFLOW` | -17 | Taşma |
| `ZEUS_ERROR_PERMISSION` | -18 | Yetki hatası |

### 3. Veri Tipleri (ZeusType)

| Tip | ID | C Karşılığı | Disk Boyutu |
|-----|----|-------------|-------------|
| `NULL` | 0 | — | 0 byte (bitmap'te) |
| `INT` | 1 | `int64_t` | 8 byte |
| `FLOAT` | 2 | `double` | 8 byte |
| `TEXT` | 3 | `char*` | 4 byte (len) + N byte |
| `BOOL` | 4 | `bool` | 1 byte |
| `BLOB` | 5 | `uint8_t*` | 4 byte (len) + N byte |
| `TIMESTAMP` | 6 | `int64_t` (unix) | 8 byte |

---

## 📦 MODÜL DETAYLARI

### MODÜL 1: Ana Header — `include/zeusdb.h` (791 satır)

Tüm veri tipleri, enum'lar, struct'lar ve fonksiyon prototipleri burada tanımlı.

**Tanımlı Enum'lar (16 adet):**
- `ZeusStatus` — 19 hata kodu
- `ZeusType` — 7 veri tipi
- `ColumnFlags` — 6 sütun flag'i (PRIMARY_KEY, NOT_NULL, UNIQUE, AUTO_INC, DEFAULT, INDEX)
- `PageType` — 7 sayfa tipi (FREE, META, TABLE_SCHEMA, BTREE_INTERNAL, BTREE_LEAF, OVERFLOW, WAL_INDEX)
- `TokenType` — ~70 SQL token tipi
- `ASTNodeType` — 14 AST düğüm tipi
- `CompareOp` — 11 karşılaştırma operatörü
- `LogicOp` — AND, OR
- `WALRecordType` — 9 WAL kayıt tipi
- `TxnState` — ACTIVE, COMMITTED, ABORTED, PREPARING
- `TxnIsolation` — 4 izolasyon seviyesi
- `LockMode` — SHARED, EXCLUSIVE, INTENT_SHARED, INTENT_EXCLUSIVE
- `LockGranularity` — DATABASE, TABLE, PAGE, ROW
- `ReplState` — 5 replikasyon durumu
- `ReplRole` — PRIMARY, REPLICA, STANDALONE

**Tanımlı Struct'lar (28 adet):**
`ZeusValue`, `ColumnDef`, `TableSchema`, `PageHeader`, `Page`, `Pager`, `BTreeCell`, `BTreeNode`, `BTree`, `Token`, `WhereClause`, `OrderByClause`, `SelectColumn`, `ASTSelect`, `ASTInsert`, `ASTUpdate`, `ASTDelete`, `ASTCreateTable`, `ASTDropTable`, `ASTNode`, `Row`, `ResultSet`, `WALRecord`, `WALManager`, `Transaction`, `TxnManager`, `LockEntry`, `LockManager`, `ReplicaInfo`, `ReplicationManager`, `Connection`, `Server`, `Database`

**Fonksiyon Prototipleri (75 adet):**
- Pager: 8 fonksiyon
- B+Tree: 7 fonksiyon
- Table: 7 fonksiyon
- SQL: 2 fonksiyon (tokenize, parse)
- Executor: 2 fonksiyon (execute, execute_sql)
- ResultSet: 4 fonksiyon
- WAL: 6 fonksiyon
- Transaction: 5 fonksiyon
- Lock Manager: 6 fonksiyon
- Replication: 7 fonksiyon
- Server: 3 fonksiyon
- Database: 2 fonksiyon
- CLI: 1 fonksiyon
- Utility: 10 fonksiyon
- Row serialization: 2 fonksiyon

---

### MODÜL 2: Storage Engine — Pager (`src/pager.c`, 301 satır)

**Görev:** Veritabanı dosyasını 4KB'lık sabit sayfalara bölerek yönetir. Tüm disk I/O bu katmandan geçer.

**Fonksiyonlar:**
| Fonksiyon | Açıklama |
|-----------|----------|
| `pager_checksum()` | FNV-1a hash (seed: 2166136261, prime: 16777619) |
| `pager_open()` | Dosya aç/oluştur, "ZEUS" magic doğrula, meta sayfa yaz |
| `pager_close()` | Flush + cache temizle + fsync + close |
| `pager_read_page()` | Cache-first okuma, double-checked locking, checksum doğrulama |
| `pager_write_page()` | Cache'e yaz, dirty işaretle (lazy write) |
| `pager_allocate_page()` | Dosyayı `pwrite` ile genişlet, yeni sayfa tahsis et |
| `pager_free_page()` | Sayfayı `PAGE_TYPE_FREE` olarak işaretle, verisini sıfırla |
| `pager_flush()` | Tüm dirty sayfaları diske yaz + checksum hesapla + fsync |

**Tasarım:**
- `pread`/`pwrite` ile positioned I/O (thread-safe)
- In-memory cache: `Page*` pointer dizisi (MAX_PAGES = 1M slot)
- `pthread_rwlock_t` ile eşzamanlı erişim
- Meta sayfa (sayfa 0): offset 0'da "ZEUS" magic, +4'te version, +8'de page_size

---

### MODÜL 2: Storage Engine — B+Tree (`src/btree.c`, 565 satır)

**Görev:** Disk tabanlı B+Tree index yapısı. Her tablo için bir PK index.

**Fonksiyonlar:**
| Fonksiyon | Görünürlük | Açıklama |
|-----------|-----------|----------|
| `btree_node_read()` | static | Sayfadan B+Tree düğümü oku |
| `btree_node_write()` | static | Düğümü sayfaya serialize edip yaz |
| `btree_node_create()` | static | Yeni sayfa tahsis et, düğüm başlat |
| `btree_find_key_pos()` | static | Binary search ile anahtar pozisyonu bul |
| `btree_create()` | public | B+Tree oluştur (leaf root ile başlar) |
| `btree_destroy()` | public | struct'u serbest bırak |
| `btree_split_leaf()` | static | Dolu leaf'i ikiye böl, key'i parent'a promote et |
| `btree_insert()` | public | Root'tan leaf'e in, ekle veya split |
| `btree_search()` | public | Root'tan leaf'e in, linear scan ile bul |
| `btree_delete()` | public | Leaf'te key bul ve sil (merge yok) |
| `btree_update()` | public | Leaf'te key bul, data_size güncelle |
| `btree_scan()` | public | Leaf'ler üzerinde range scan (linked list) |

**Node Serialization Formatı:**
```
[num_keys: u32][next_leaf: u32][prev_leaf: u32]
[keys: u32 × num_keys]
Leaf:     [cell: {key:u32, data_offset:u32, data_size:u16} × num_keys]
Internal: [children: u32 × (num_keys+1)]
```

**Tasarım Notları:**
- Leaf düğümler çift yönlü bağlı (next_leaf, prev_leaf) → range scan için
- Split sadece leaf'te uygulanmış; internal node split basitleştirilmiş
- Delete'te merge/rebalance **YOK** (TODO olarak işaretli)
- `pthread_rwlock_t` — read: rdlock, write: wrlock

---

### MODÜL 3: SQL Tokenizer (`src/tokenizer.c`, 340 satır)

**Görev:** SQL string'ini token dizisine ayırır.

**Desteklenen Tokenlar:**
- **Anahtar kelimeler:** 80+ keyword (SELECT, FROM, WHERE, INSERT, INTO, VALUES, UPDATE, SET, DELETE, CREATE, TABLE, DROP, ALTER, ADD, INDEX, PRIMARY, KEY, NOT, NULL, UNIQUE, DEFAULT, AUTO_INCREMENT, INT/INTEGER/BIGINT, FLOAT/DOUBLE/REAL, TEXT/CHAR, VARCHAR, BOOL/BOOLEAN, BLOB, TIMESTAMP/DATETIME, AND, OR, ORDER, BY, ASC, DESC, LIMIT, OFFSET, JOIN, ON, INNER, LEFT, RIGHT, GROUP, HAVING, AS, BEGIN, COMMIT, ROLLBACK, SAVEPOINT, COUNT, SUM, AVG, MIN, MAX, IF, EXISTS, IN, BETWEEN, LIKE, IS, TRUE, FALSE, SHOW, TABLES, DESCRIBE, EXPLAIN)
- **Literal'lar:** Integer, Float, String (tek/çift tırnak), NULL, TRUE, FALSE
- **Operatörler:** `=`, `!=`, `<>`, `<`, `>`, `<=`, `>=`
- **Noktalama:** `(`, `)`, `,`, `;`, `*`, `.`, `+`, `-`, `/`
- **Yorumlar:** `-- tek satır` ve `/* çok satır */`
- **Escape:** `\'` (tek tırnak içinde)

**Özellikler:**
- Case-insensitive keyword eşleme (upper'a çevirip karşılaştırma)
- Satır ve sütun takibi (hata raporlama için)
- Her token dizisi `TOK_EOF` ile biter

---

### MODÜL 3: SQL Parser (`src/parser.c`, 843 satır)

**Görev:** Token dizisini AST (Abstract Syntax Tree) yapısına çevirir. Recursive descent parser.

**Desteklenen SQL Komutları:**

| Komut | Detaylar |
|-------|----------|
| `SELECT` | `*`, sütun seçimi, `table.column`, alias (`AS`), aggregate (COUNT/SUM/AVG/MIN/MAX), FROM, WHERE, ORDER BY (ASC/DESC), LIMIT, OFFSET |
| `INSERT` | `INSERT INTO tablo (sütunlar) VALUES (değerler)` veya `INSERT INTO tablo VALUES (...)` |
| `UPDATE` | `UPDATE tablo SET col=val, ... WHERE ...` |
| `DELETE` | `DELETE FROM tablo WHERE ...` |
| `CREATE TABLE` | `IF NOT EXISTS`, sütun tanımları, veri tipleri, constraint'ler (PK, NOT NULL, UNIQUE, AUTO_INCREMENT, DEFAULT), tablo seviyesi PRIMARY KEY |
| `DROP TABLE` | `IF EXISTS` desteği |
| `BEGIN` | Transaction başlat |
| `COMMIT` | Transaction onayla |
| `ROLLBACK` | Transaction geri al |
| `SHOW TABLES` | Tablo listesi |
| `DESCRIBE tablo` | Tablo yapısını göster |
| `EXPLAIN SELECT` | Sorgu planı |

**WHERE Desteği:**
- Karşılaştırma: `=`, `!=`, `<`, `>`, `<=`, `>=`
- Pattern: `LIKE '%pattern%'`
- Aralık: `BETWEEN x AND y`
- Null: `IS NULL`, `IS NOT NULL`
- Mantık: `AND`, `OR` zincirleme

**Desteklenen Veri Tipleri:**
- `INT`, `INTEGER`, `BIGINT` → ZEUS_TYPE_INT
- `FLOAT`, `DOUBLE`, `REAL` → ZEUS_TYPE_FLOAT
- `TEXT`, `CHAR` → ZEUS_TYPE_TEXT
- `VARCHAR(n)` → ZEUS_TYPE_TEXT (max_len ile)
- `BOOL`, `BOOLEAN` → ZEUS_TYPE_BOOL
- `BLOB` → ZEUS_TYPE_BLOB
- `TIMESTAMP`, `DATETIME` → ZEUS_TYPE_TIMESTAMP

---

### MODÜL 4: Table Manager & Utilities (`src/table.c`, 732 satır)

**Görev:** Tablo CRUD, satır serialize/deserialize, yardımcı fonksiyonlar.

**İçerdiği Alt Sistemler:**

**a) Utility Fonksiyonları:**
- `zeus_status_str()` — Hata kodu → Türkçe mesaj
- `zeus_log()` — Zaman damgalı loglama (stderr, va_list)
- `zeus_timestamp_us()` — Mikrosaniye hassasiyetinde zaman (CLOCK_MONOTONIC)

**b) Value Fonksiyonları:**
- `zeus_value_free/int/float/text/bool/null()` — Değer oluşturma/yıkma
- `zeus_value_compare()` — Karşılaştırma (NULL handling, INT↔FLOAT coercion)
- `zeus_value_copy()` — Derin kopya (TEXT için strdup, BLOB için malloc+memcpy)

**c) Row Serialization Formatı:**
```
[rowid: uint32_t (4 byte)]
[null_bitmap: ceil(num_columns/8) byte]
[sütun verileri...]
  INT/TIMESTAMP: 8 byte (int64_t)
  FLOAT: 8 byte (double)  
  TEXT: [uzunluk: uint32_t (4 byte)][karakterler: N byte]
  BOOL: 1 byte
  NULL sütunlar: 0 byte (bitmap'te işaretli)
```

**d) ResultSet:**
- Dinamik dizi (başlangıç kapasitesi: 64, 2x büyüme)
- Pretty-print: Unicode box-drawing karakterleri ile tablo çıktısı (┌─┬┐│├┼┤└┴┘)
- Sütun genişliği otomatik hesaplama (maks 50 karakter)

**e) Table Manager:**
- `table_find()` — Case-insensitive linear search
- `table_create()` — Schema kaydet, B+Tree oluştur, schema page'e yaz
- `table_drop()` — B+Tree yık, diziden çıkar (sola kaydır)
- `table_insert_row()` — Auto-increment rowid, serialize, B+Tree'ye ekle, data page'e yaz
- `table_delete_row()` — B+Tree'den sil, row_count azalt
- `table_scan()` — Tüm BTREE_LEAF page'leri tara, filtre uygula

---

### MODÜL 4: Query Executor (`src/executor.c`, 880 satır)

**Görev:** AST'yi alıp veritabanı üzerinde çalıştırır.

**Çalıştırma Pipeline'ı:**
```
SQL string → tokenize() → Token[] → parse() → ASTNode → execute() → ResultSet
```

Kısayol: `execute_sql()` hepsini tek çağrıda yapar.

**Fonksiyonlar:**
| Fonksiyon | Açıklama |
|-----------|----------|
| `like_match()` | `%` (herhangi) ve `_` (tek karakter) pattern matching, case-insensitive |
| `evaluate_condition()` | Tek WHERE koşulunu değerlendir |
| `evaluate_where()` | AND/OR zincirleme koşulları değerlendir |
| `exec_create_table()` | table_create çağır, mesaj döndür |
| `exec_drop_table()` | table_drop çağır, IF EXISTS handle |
| `exec_insert()` | Sütun eşleme, NOT NULL/AUTO_INC kontrol, PK→rowid, insert |
| `exec_select()` | Full scan → WHERE filtre → Aggregate → ORDER BY (qsort) → OFFSET/LIMIT → Projection |
| `exec_update()` | Page scan → WHERE → SET uygula → Re-serialize → Write back |
| `exec_delete()` | Page scan → WHERE → Cell'i sıfırla → num_cells azalt |
| `exec_show_tables()` | Tüm tabloları listele (ad, sütun sayısı, satır sayısı) |
| `exec_describe()` | Sütun bilgileri göster (ad, tip, nullable, key, extra) |
| `execute()` | Ana dağıtıcı + süre ölçümü |
| `execute_sql()` | Tokenize + Parse + Execute pipeline |

**SELECT Çalışma Akışı:**
1. Tüm leaf page'leri tara (full table scan)
2. WHERE filtresi uygula
3. Aggregate varsa hesapla (COUNT/SUM/AVG/MIN/MAX)
4. ORDER BY varsa qsort ile sırala
5. OFFSET atla, LIMIT uygula
6. Sütun projection (seçili sütunları çek)

**Önemli Not:** SELECT şu an B+Tree index'ini kullanmıyor, full page scan yapıyor. İlerleyen aşamada index-based lookup eklenebilir.

---

### MODÜL 5: WAL — Write-Ahead Log (`src/wal.c`, 368 satır)

**Görev:** ACID'in D'si (Durability). Tüm değişiklikler önce WAL'a yazılır, crash sonrası recovery sağlar.

**WAL Record Formatı (disk üzerinde):**
```
[WALRecord header]
  lsn: uint64_t         — Log Sequence Number (monoton artan)
  type: WALRecordType    — INSERT/UPDATE/DELETE/CREATE/DROP/CHECKPOINT/TXN_BEGIN/COMMIT/ROLLBACK
  txn_id: uint64_t       — Transaction ID
  table_id: uint32_t     — Tablo ID
  page_num: uint32_t     — Etkilenen sayfa
  data_len: uint32_t     — Veri uzunluğu
  checksum: uint32_t     — FNV-1a (header XOR data)
  timestamp: uint64_t    — Unix timestamp
[data: data_len byte]
```

**Segment Yapısı:**
- Her segment 16MB
- Dosya isimlendirme: `wal_XXXXXXXX.log` (8 haneli segment numarası)
- Segment dolunca yenisi açılır (rotate)
- Checkpoint: dirty page'ler diske yazılır, eski segmentler silinir

**Fonksiyonlar:**
| Fonksiyon | Açıklama |
|-----------|----------|
| `wal_open()` | Dizin oluştur, 16MB buffer tahsis et, mevcut LSN'yi oku |
| `wal_close()` | Flush + fsync + close + free |
| `wal_write()` | LSN ata, checksum hesapla, buffer'a yaz, segment dolunca rotate |
| `wal_flush()` | Buffer'ı diske yaz + fsync |
| `wal_checkpoint()` | Pager flush + CHECKPOINT kaydı yaz + eski segmentleri sil |
| `wal_recover()` | Checkpoint sonrası tüm kayıtları oku, checksum doğrula, sayfalara uygula |

**Tasarım:**
- Buffered write (16MB buffer), overflow veya explicit flush'ta diske yazılır
- Checksum: header ve data'nın FNV-1a hash'lerinin XOR'u
- Recovery: checkpoint_lsn sonrası tüm kayıtlar tekrar oynatılır (redo)
- `pthread_mutex_t` ile thread-safe

---

### MODÜL 5: Transaction Manager (`src/txn.c`, 196 satır)

**Görev:** ACID transaction yönetimi.

**Fonksiyonlar:**
| Fonksiyon | Açıklama |
|-----------|----------|
| `txn_mgr_init()` | Mutex init, next_txn_id=1 |
| `txn_mgr_destroy()` | Tüm aktif txn'ları rollback et, free |
| `txn_begin()` | Boş slot bul, ID ata, WAL_TXN_BEGIN yaz |
| `txn_commit()` | WAL_TXN_COMMIT yaz, WAL flush (durability), state=COMMITTED |
| `txn_rollback()` | WAL_TXN_ROLLBACK yaz, flush, state=ABORTED |

**Tasarım:**
- Slot tabanlı depolama: 1024 Transaction struct dizisi, linear scan ile boş slot bulma
- Varsayılan izolasyon: READ_COMMITTED
- Commit'te `wal_flush()` çağrılır → WAL diske yazılır → durability garantisi
- **Undo logging henüz yok!** Rollback WAL kaydı yazıyor ama gerçek geri alma yapMIYOR (TODO)
- `pthread_mutex_t` ile thread-safe

---

## 🔧 İMPLEMENTASYON BEKLEYENLERİN DETAYLARI

### 6. Lock Manager (`src/lock.c` — YAZILACAK)
Header'da tanımlı prototipler:
```c
ZeusStatus lock_mgr_init(LockManager **mgr);
ZeusStatus lock_mgr_destroy(LockManager *mgr);
ZeusStatus lock_acquire(LockManager *mgr, uint64_t txn_id, uint32_t resource_id,
                         LockMode mode, LockGranularity granularity);
ZeusStatus lock_release(LockManager *mgr, uint64_t txn_id, uint32_t resource_id);
ZeusStatus lock_release_all(LockManager *mgr, uint64_t txn_id);
bool       lock_detect_deadlock(LockManager *mgr, uint64_t txn_id);
```
**Plan:**
- Row-level locking (fine-grained)
- Shared (read) ve Exclusive (write) lock
- Intent lock'lar ile hiyerarşik kilitleme
- Wait-for graph ile deadlock detection
- Timeout: 30 saniye (LOCK_TIMEOUT_MS)

### 7. Replication Manager (`src/replication.c` — YAZILACAK)
Header'da tanımlı prototipler:
```c
ZeusStatus repl_init(ReplicationManager **mgr, WALManager *wal, ReplRole role);
ZeusStatus repl_destroy(ReplicationManager *mgr);
ZeusStatus repl_add_replica(ReplicationManager *mgr, const char *host, uint16_t port);
ZeusStatus repl_remove_replica(ReplicationManager *mgr, const char *host, uint16_t port);
ZeusStatus repl_start(ReplicationManager *mgr);
ZeusStatus repl_stop(ReplicationManager *mgr);
ZeusStatus repl_send_wal(ReplicationManager *mgr, const WALRecord *record, const uint8_t *data);
```
**Plan:**
- Primary-Replica (master-slave) mimarisi
- WAL tabanlı replikasyon (WAL kayıtları replica'ya gönderilir)
- Senkron/Asenkron seçenek
- Heartbeat: 5 saniye aralıklarla (REPL_HEARTBEAT_MS)
- Maks 8 replika (MAX_REPLICAS)

### 8. Server & CLI (`src/server.c`, `src/cli.c`, `src/main.c` — YAZILACAK)
Header'da tanımlı prototipler:
```c
ZeusStatus server_init(Server **srv, uint16_t port);
ZeusStatus server_start(Server *srv, Database *db);
ZeusStatus server_stop(Server *srv);
ZeusStatus db_open(Database **db, const char *name, const char *data_dir);
ZeusStatus db_close(Database *db);
void       cli_run(Database *db);
```
**Plan:**
- TCP socket server (maks 256 bağlantı)
- Thread pool ile bağlantı yönetimi (16 thread)
- İnteraktif CLI (REPL — Read-Eval-Print Loop)
- `db_open`: Pager + WAL + TxnMgr + LockMgr + ReplMgr başlat, WAL recovery çalıştır
- `db_close`: Tüm bileşenleri kapat, flush, fsync

### 9. Makefile — YAZILACAK
**Plan:**
- `make` ile derleme (gcc veya clang)
- `make clean` ile temizleme
- `make test` ile testleri çalıştır
- Compiler flag'ler: `-Wall -Wextra -O2 -pthread`

### 10. Testler — YAZILACAK
**Plan:**
- Birim testler: Her modül için ayrı test dosyası
- Entegrasyon testleri: SQL'den sonuca kadar end-to-end
- Stress test: Eşzamanlı erişim, yüksek yük
- Recovery test: Crash simülasyonu + WAL recovery

---

## 📊 ANA VERİ YAPILARI DİYAGRAMI

```
Database
├── name, data_dir
├── Pager* ──► Disk I/O (4KB sayfalar)
│   ├── page_cache[1M] ──► Page*
│   ├── dirty[1M]
│   └── fd (dosya descriptor)
├── tables[256] ──► TableSchema
│   ├── name, columns[64], num_columns
│   ├── primary_key_col, next_rowid
│   └── root_page (B+Tree root)
├── indexes[256] ──► BTree*
│   ├── root_page, num_entries, height
│   └── Pager* (paylaşımlı)
├── WALManager*
│   ├── current_lsn, flushed_lsn, checkpoint_lsn
│   ├── buffer[16MB], buffer_pos
│   └── segment dosyaları: wal_XXXXXXXX.log
├── TxnManager*
│   ├── transactions[1024]
│   └── next_txn_id
├── LockManager* (YAZILACAK)
├── ReplicationManager* (YAZILACAK)
└── Server* (YAZILACAK)
```

---

## 🔀 SQL ÇALIŞMA AKIŞI

```
Kullanıcı SQL yazıyor
        │
        ▼
┌─────────────────┐
│   Tokenizer     │  SQL string → Token dizisi
│  tokenizer.c    │  (keyword, literal, operator, punctuation)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    Parser       │  Token dizisi → AST (Abstract Syntax Tree)
│   parser.c      │  (recursive descent)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Executor      │  AST → Veritabanı işlemleri → ResultSet
│  executor.c     │
├─────────────────┤
│  ┌─ table.c ──┐ │  Tablo yönetimi, row serialize
│  ├─ btree.c ──┤ │  Index insert/search/delete
│  ├─ pager.c ──┤ │  Disk I/O
│  ├─ wal.c ────┤ │  Write-Ahead Log
│  └─ txn.c ────┘ │  Transaction yönetimi
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   ResultSet     │  Sonuçlar → Pretty-print tablo
│   table.c       │  (Unicode box-drawing)
└─────────────────┘
```

---

## ⚠️ BİLİNEN EKSİKLİKLER VE TODO'LAR

1. **B+Tree internal node split** — Recursive split tam uygulanmadı (basitleştirilmiş)
2. **B+Tree delete merge/rebalance** — Silme sonrası dengeleme yok
3. **Undo logging** — Transaction rollback gerçek geri alma yapmıyor
4. **SELECT index kullanımı** — Full table scan yapıyor, B+Tree index lookup yok
5. **ORDER BY sort** — Global `SortCtx*` kullanıyor, thread-safe değil
6. **Lock Manager** — Henüz yazılmadı
7. **Replication** — Henüz yazılmadı
8. **Server/CLI** — Henüz yazılmadı
9. **JOIN desteği** — Parser'da yapı var ama executor'da yok
10. **GROUP BY / HAVING** — Token var ama implement yok
11. **Sub-query** — Yok
12. **Foreign key** — Yok
13. **table_update_row()** — Header'da tanımlı ama implementasyonu yok

---

## 🔑 ÖNEMLİ TASARIM KARARLARI

1. **Pure C** — Sıfır bağımlılık (sadece POSIX + pthread)
2. **4KB sayfa** — SSD/HDD block size'a uyumlu
3. **FNV-1a checksum** — Hızlı, iyi dağılımlı hash
4. **WAL-first** — Tüm değişiklikler önce WAL'a, sonra data page'lere
5. **Lazy write** — page_cache + dirty flag, flush'ta toplu yazma
6. **B+Tree leaf linking** — Linked list ile range scan O(k) (k = sonuç sayısı)
7. **Row-oriented storage** — Her satır tek parça olarak kaydedilir
8. **Null bitmap** — Her sütun için 1 bit, NULL sütunlar disk alanı harcamaz

---

*Bu dosya ZeusDB projesinin tam hafızasıdır. Projeye her devam ettiğinde bu dosyayı oku.*
