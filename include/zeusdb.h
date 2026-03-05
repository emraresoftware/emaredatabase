/*
 * ZeusDB - High-Performance Relational Database Engine
 * Copyright (c) 2026 - Built to surpass the titans
 *
 * Ana header dosyası - Tüm veri tipleri ve sabitler
 */

#ifndef ZEUSDB_H
#define ZEUSDB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

/* ============================================================
 * SABITLER
 * ============================================================ */

#define ZEUS_VERSION          "1.0.0"
#define ZEUS_VERSION_MAJOR    1
#define ZEUS_VERSION_MINOR    0
#define ZEUS_VERSION_PATCH    0

/* Sayfa ve Bellek */
#define PAGE_SIZE             4096
#define MAX_PAGES             1048576    /* 4GB max database */
#define PAGE_HEADER_SIZE      64
#define MAX_ROW_SIZE          (PAGE_SIZE - PAGE_HEADER_SIZE - 16)

/* B+Tree */
#define BTREE_ORDER           128
#define BTREE_MAX_KEYS        (BTREE_ORDER - 1)
#define BTREE_MIN_KEYS        (BTREE_ORDER / 2 - 1)
#define BTREE_MAX_CHILDREN    BTREE_ORDER

/* Tablo ve Sütun */
#define MAX_COLUMNS           64
#define MAX_COLUMN_NAME       64
#define MAX_TABLE_NAME        128
#define MAX_TABLES            256
#define MAX_INDEX_NAME        128
#define MAX_INDEXES_PER_TABLE 16
#define MAX_VARCHAR_LEN       4096

/* SQL */
#define MAX_SQL_LENGTH        65536
#define MAX_TOKENS            4096
#define MAX_TOKEN_LENGTH      512
#define MAX_WHERE_CONDITIONS  32
#define MAX_ORDER_BY          8
#define MAX_JOIN_TABLES       8

/* Transaction */
#define MAX_TRANSACTIONS      1024
#define WAL_SEGMENT_SIZE      (16 * 1024 * 1024) /* 16MB WAL segment */
#define MAX_WAL_SEGMENTS      256
#define CHECKPOINT_INTERVAL   1000  /* her 1000 transaction'da 1 checkpoint */

/* Concurrency */
#define MAX_CONNECTIONS       256
#define MAX_LOCKS             4096
#define LOCK_TIMEOUT_MS       30000  /* 30 saniye */
#define THREAD_POOL_SIZE      16

/* Replication */
#define MAX_REPLICAS          8
#define REPL_BUFFER_SIZE      (1024 * 1024)  /* 1MB buffer */
#define REPL_HEARTBEAT_MS     5000

/* Logging */
#define LOG_BUFFER_SIZE       8192

/* ============================================================
 * HATA KODLARI
 * ============================================================ */

typedef enum {
    ZEUS_OK = 0,
    ZEUS_ERROR = -1,
    ZEUS_ERROR_IO = -2,
    ZEUS_ERROR_CORRUPT = -3,
    ZEUS_ERROR_FULL = -4,
    ZEUS_ERROR_NOT_FOUND = -5,
    ZEUS_ERROR_EXISTS = -6,
    ZEUS_ERROR_CONSTRAINT = -7,
    ZEUS_ERROR_TYPE = -8,
    ZEUS_ERROR_SYNTAX = -9,
    ZEUS_ERROR_INTERNAL = -10,
    ZEUS_ERROR_TXN_ABORT = -11,
    ZEUS_ERROR_DEADLOCK = -12,
    ZEUS_ERROR_TIMEOUT = -13,
    ZEUS_ERROR_READONLY = -14,
    ZEUS_ERROR_NOMEM = -15,
    ZEUS_ERROR_BUSY = -16,
    ZEUS_ERROR_OVERFLOW = -17,
    ZEUS_ERROR_PERMISSION = -18,
} ZeusStatus;

/* ============================================================
 * VERİ TİPLERİ
 * ============================================================ */

typedef enum {
    ZEUS_TYPE_NULL = 0,
    ZEUS_TYPE_INT,        /* 64-bit signed integer */
    ZEUS_TYPE_FLOAT,      /* 64-bit double */
    ZEUS_TYPE_TEXT,        /* Variable length string */
    ZEUS_TYPE_BOOL,       /* Boolean */
    ZEUS_TYPE_BLOB,       /* Binary data */
    ZEUS_TYPE_TIMESTAMP,  /* Unix timestamp */
} ZeusType;

typedef struct {
    ZeusType type;
    union {
        int64_t  int_val;
        double   float_val;
        bool     bool_val;
        int64_t  timestamp_val;
        struct {
            char    *data;
            uint32_t len;
        } str_val;
        struct {
            uint8_t *data;
            uint32_t len;
        } blob_val;
    };
    bool is_null;
} ZeusValue;

/* ============================================================
 * SÜTUN TANIMI
 * ============================================================ */

typedef enum {
    COL_FLAG_NONE        = 0,
    COL_FLAG_PRIMARY_KEY = (1 << 0),
    COL_FLAG_NOT_NULL    = (1 << 1),
    COL_FLAG_UNIQUE      = (1 << 2),
    COL_FLAG_AUTO_INC    = (1 << 3),
    COL_FLAG_DEFAULT     = (1 << 4),
    COL_FLAG_INDEX       = (1 << 5),
} ColumnFlags;

typedef struct {
    char       name[MAX_COLUMN_NAME];
    ZeusType   type;
    uint32_t   max_len;           /* TEXT/BLOB için max uzunluk */
    uint32_t   flags;
    ZeusValue  default_val;
    int32_t    col_index;         /* Sütun sırası */
} ColumnDef;

/* ============================================================
 * TABLO ŞEMASI
 * ============================================================ */

typedef struct {
    char       name[MAX_TABLE_NAME];
    ColumnDef  columns[MAX_COLUMNS];
    uint32_t   num_columns;
    uint32_t   primary_key_col;   /* PK sütun indexi */
    uint32_t   row_count;
    uint32_t   next_rowid;        /* AUTO_INCREMENT counter */
    uint32_t   root_page;         /* B+Tree root page numarası */
    bool       has_primary_key;
} TableSchema;

/* ============================================================
 * SAYFA YÖNETİMİ (PAGER)
 * ============================================================ */

typedef enum {
    PAGE_TYPE_FREE = 0,
    PAGE_TYPE_META,         /* Veritabanı metadata */
    PAGE_TYPE_TABLE_SCHEMA, /* Tablo şema bilgisi */
    PAGE_TYPE_BTREE_INTERNAL,
    PAGE_TYPE_BTREE_LEAF,
    PAGE_TYPE_OVERFLOW,     /* Büyük veriler için taşma sayfası */
    PAGE_TYPE_WAL_INDEX,
} PageType;

typedef struct {
    uint32_t  page_num;
    PageType  type;
    uint32_t  num_cells;        /* Bu sayfadaki kayıt sayısı */
    uint32_t  free_space;       /* Boş alan (byte) */
    uint32_t  next_page;        /* Bağlı sayfa (overflow/next leaf) */
    uint32_t  parent_page;      /* Üst sayfa (B+Tree için) */
    uint32_t  checksum;         /* Bütünlük kontrolü */
    uint64_t  lsn;              /* Log Sequence Number (WAL) */
    uint8_t   reserved[20];     /* Gelecek kullanım */
} PageHeader;

typedef struct {
    PageHeader header;
    uint8_t    data[PAGE_SIZE - sizeof(PageHeader)];
} Page;

typedef struct {
    int        fd;              /* Dosya descriptor */
    char       filepath[512];
    uint32_t   num_pages;
    uint32_t   file_size;
    Page      *page_cache[MAX_PAGES]; /* Basit sayfa cache */
    bool       dirty[MAX_PAGES];      /* Değişmiş sayfalar */
    pthread_rwlock_t lock;
} Pager;

/* ============================================================
 * B+TREE
 * ============================================================ */

typedef struct {
    uint32_t key;               /* rowid */
    uint32_t data_offset;       /* Sayfa içi offset */
    uint16_t data_size;
} BTreeCell;

typedef struct {
    uint32_t    page_num;
    bool        is_leaf;
    uint32_t    num_keys;
    uint32_t    keys[BTREE_MAX_KEYS];
    uint32_t    children[BTREE_MAX_CHILDREN]; /* Internal: child page nums */
    BTreeCell   cells[BTREE_MAX_KEYS];         /* Leaf: actual cells */
    uint32_t    next_leaf;                      /* Leaf: sonraki yaprak */
    uint32_t    prev_leaf;                      /* Leaf: önceki yaprak */
    uint32_t    parent;
} BTreeNode;

typedef struct {
    Pager      *pager;
    uint32_t    root_page;
    uint32_t    num_entries;
    uint32_t    height;
    pthread_rwlock_t lock;
} BTree;

/* ============================================================
 * SQL TOKEN / AST
 * ============================================================ */

typedef enum {
    /* Keywords */
    TOK_SELECT, TOK_FROM, TOK_WHERE, TOK_INSERT, TOK_INTO,
    TOK_VALUES, TOK_UPDATE, TOK_SET, TOK_DELETE, TOK_CREATE,
    TOK_TABLE, TOK_DROP, TOK_ALTER, TOK_ADD, TOK_INDEX,
    TOK_PRIMARY, TOK_KEY, TOK_NOT, TOK_NULL_KW, TOK_UNIQUE,
    TOK_DEFAULT, TOK_AUTO_INCREMENT, TOK_INT_KW, TOK_FLOAT_KW,
    TOK_TEXT_KW, TOK_BOOL_KW, TOK_BLOB_KW, TOK_TIMESTAMP_KW,
    TOK_AND, TOK_OR, TOK_ORDER, TOK_BY, TOK_ASC, TOK_DESC,
    TOK_LIMIT, TOK_OFFSET, TOK_JOIN, TOK_ON, TOK_INNER,
    TOK_LEFT, TOK_RIGHT, TOK_GROUP, TOK_HAVING, TOK_AS,
    TOK_BEGIN, TOK_COMMIT, TOK_ROLLBACK, TOK_SAVEPOINT,
    TOK_COUNT, TOK_SUM, TOK_AVG, TOK_MIN_KW, TOK_MAX_KW,
    TOK_IF, TOK_EXISTS, TOK_IN, TOK_BETWEEN, TOK_LIKE,
    TOK_IS, TOK_TRUE_KW, TOK_FALSE_KW, TOK_VARCHAR,
    TOK_SHOW, TOK_TABLES, TOK_DESCRIBE, TOK_EXPLAIN,

    /* Literals & Identifiers */
    TOK_IDENTIFIER,
    TOK_INTEGER_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,

    /* Operators */
    TOK_EQ,          /* = */
    TOK_NEQ,         /* != veya <> */
    TOK_LT,          /* < */
    TOK_GT,          /* > */
    TOK_LTE,         /* <= */
    TOK_GTE,         /* >= */

    /* Punctuation */
    TOK_LPAREN,      /* ( */
    TOK_RPAREN,      /* ) */
    TOK_COMMA,       /* , */
    TOK_SEMICOLON,   /* ; */
    TOK_STAR,        /* * */
    TOK_DOT,         /* . */
    TOK_PLUS,        /* + */
    TOK_MINUS,       /* - */
    TOK_SLASH,       /* / */

    /* Special */
    TOK_EOF,
    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    char      value[MAX_TOKEN_LENGTH];
    uint32_t  line;
    uint32_t  col;
} Token;

/* AST Node Types */
typedef enum {
    AST_SELECT,
    AST_INSERT,
    AST_UPDATE,
    AST_DELETE,
    AST_CREATE_TABLE,
    AST_DROP_TABLE,
    AST_CREATE_INDEX,
    AST_DROP_INDEX,
    AST_BEGIN,
    AST_COMMIT,
    AST_ROLLBACK,
    AST_SHOW_TABLES,
    AST_DESCRIBE,
    AST_EXPLAIN,
} ASTNodeType;

/* WHERE koşulu */
typedef enum {
    CMP_EQ,
    CMP_NEQ,
    CMP_LT,
    CMP_GT,
    CMP_LTE,
    CMP_GTE,
    CMP_LIKE,
    CMP_IN,
    CMP_BETWEEN,
    CMP_IS_NULL,
    CMP_IS_NOT_NULL,
} CompareOp;

typedef enum {
    LOGIC_AND,
    LOGIC_OR,
} LogicOp;

typedef struct {
    char       column[MAX_COLUMN_NAME];
    CompareOp  op;
    ZeusValue  value;
    ZeusValue  value2;      /* BETWEEN için ikinci değer */
    LogicOp    logic;       /* Sonraki koşulla bağlantı */
} WhereClause;

/* ORDER BY */
typedef struct {
    char   column[MAX_COLUMN_NAME];
    bool   descending;
} OrderByClause;

/* SELECT column */
typedef struct {
    char   name[MAX_COLUMN_NAME];
    char   alias[MAX_COLUMN_NAME];
    char   table[MAX_TABLE_NAME];    /* table.column için */
    bool   is_star;                  /* SELECT * */
    bool   is_aggregate;
    TokenType aggregate_func;        /* COUNT, SUM, AVG, MIN, MAX */
} SelectColumn;

/* AST: SELECT statement */
typedef struct {
    SelectColumn  columns[MAX_COLUMNS];
    uint32_t      num_columns;
    char          table_name[MAX_TABLE_NAME];
    WhereClause   where[MAX_WHERE_CONDITIONS];
    uint32_t      num_where;
    OrderByClause order_by[MAX_ORDER_BY];
    uint32_t      num_order_by;
    int64_t       limit;
    int64_t       offset;
    bool          has_where;
    bool          has_order_by;
    bool          has_limit;
    bool          has_offset;
    bool          explain;
} ASTSelect;

/* AST: INSERT statement */
typedef struct {
    char          table_name[MAX_TABLE_NAME];
    char          columns[MAX_COLUMNS][MAX_COLUMN_NAME];
    uint32_t      num_columns;
    ZeusValue     values[MAX_COLUMNS];
    uint32_t      num_values;
    bool          has_columns;     /* INSERT INTO t (col, ..) veya INSERT INTO t */
} ASTInsert;

/* AST: UPDATE statement */
typedef struct {
    char          table_name[MAX_TABLE_NAME];
    char          set_columns[MAX_COLUMNS][MAX_COLUMN_NAME];
    ZeusValue     set_values[MAX_COLUMNS];
    uint32_t      num_sets;
    WhereClause   where[MAX_WHERE_CONDITIONS];
    uint32_t      num_where;
    bool          has_where;
} ASTUpdate;

/* AST: DELETE statement */
typedef struct {
    char          table_name[MAX_TABLE_NAME];
    WhereClause   where[MAX_WHERE_CONDITIONS];
    uint32_t      num_where;
    bool          has_where;
} ASTDelete;

/* AST: CREATE TABLE */
typedef struct {
    char          table_name[MAX_TABLE_NAME];
    ColumnDef     columns[MAX_COLUMNS];
    uint32_t      num_columns;
    bool          if_not_exists;
} ASTCreateTable;

/* AST: DROP TABLE */
typedef struct {
    char          table_name[MAX_TABLE_NAME];
    bool          if_exists;
} ASTDropTable;

/* Genel AST düğümü */
typedef struct {
    ASTNodeType type;
    union {
        ASTSelect      select_stmt;
        ASTInsert      insert_stmt;
        ASTUpdate      update_stmt;
        ASTDelete      delete_stmt;
        ASTCreateTable create_table;
        ASTDropTable   drop_table;
        char           table_name[MAX_TABLE_NAME]; /* DESCRIBE, SHOW için */
    };
} ASTNode;

/* ============================================================
 * SATIR (ROW) TANIMI
 * ============================================================ */

typedef struct {
    uint32_t   rowid;
    ZeusValue  values[MAX_COLUMNS];
    uint32_t   num_values;
} Row;

/* Sorgu sonuç kümesi */
typedef struct {
    Row       *rows;
    uint32_t   num_rows;
    uint32_t   capacity;
    char       col_names[MAX_COLUMNS][MAX_COLUMN_NAME];
    ZeusType   col_types[MAX_COLUMNS];
    uint32_t   num_cols;
    uint64_t   exec_time_us;     /* Çalışma süresi (mikrosaniye) */
    uint32_t   rows_affected;
} ResultSet;

/* ============================================================
 * WAL (Write-Ahead Log)
 * ============================================================ */

typedef enum {
    WAL_INSERT,
    WAL_UPDATE,
    WAL_DELETE,
    WAL_CREATE_TABLE,
    WAL_DROP_TABLE,
    WAL_CHECKPOINT,
    WAL_TXN_BEGIN,
    WAL_TXN_COMMIT,
    WAL_TXN_ROLLBACK,
} WALRecordType;

typedef struct {
    uint64_t       lsn;           /* Log Sequence Number */
    WALRecordType  type;
    uint64_t       txn_id;
    uint32_t       table_id;
    uint32_t       page_num;
    uint32_t       data_len;
    uint32_t       checksum;
    uint64_t       timestamp;
    /* Ardından data_len byte veri gelir */
} WALRecord;

typedef struct {
    int            fd;
    char           dirpath[512];
    uint64_t       current_lsn;
    uint64_t       flushed_lsn;
    uint64_t       checkpoint_lsn;
    uint32_t       current_segment;
    uint8_t       *buffer;
    uint32_t       buffer_pos;
    uint32_t       buffer_size;
    pthread_mutex_t lock;
} WALManager;

/* ============================================================
 * TRANSACTION MANAGER
 * ============================================================ */

typedef enum {
    TXN_ACTIVE,
    TXN_COMMITTED,
    TXN_ABORTED,
    TXN_PREPARING,    /* 2-Phase Commit için */
} TxnState;

typedef enum {
    TXN_ISOLATION_READ_UNCOMMITTED,
    TXN_ISOLATION_READ_COMMITTED,
    TXN_ISOLATION_REPEATABLE_READ,
    TXN_ISOLATION_SERIALIZABLE,
} TxnIsolation;

typedef struct {
    uint64_t       txn_id;
    TxnState       state;
    TxnIsolation   isolation;
    uint64_t       start_lsn;
    uint64_t       start_timestamp;
    uint32_t       num_locks;
    bool           read_only;
} Transaction;

typedef struct {
    Transaction     transactions[MAX_TRANSACTIONS];
    uint32_t        num_active;
    uint64_t        next_txn_id;
    WALManager     *wal;
    pthread_mutex_t lock;
} TxnManager;

/* ============================================================
 * LOCK MANAGER (Concurrency)
 * ============================================================ */

typedef enum {
    LOCK_SHARED,        /* Read lock */
    LOCK_EXCLUSIVE,     /* Write lock */
    LOCK_INTENT_SHARED,
    LOCK_INTENT_EXCLUSIVE,
} LockMode;

typedef enum {
    LOCK_GRANULARITY_DATABASE,
    LOCK_GRANULARITY_TABLE,
    LOCK_GRANULARITY_PAGE,
    LOCK_GRANULARITY_ROW,
} LockGranularity;

typedef struct {
    uint64_t        txn_id;
    uint32_t        resource_id;   /* Tablo/sayfa/satır ID */
    LockMode        mode;
    LockGranularity granularity;
    bool            granted;
    struct timespec  acquired_at;
} LockEntry;

typedef struct {
    LockEntry       locks[MAX_LOCKS];
    uint32_t        num_locks;
    pthread_mutex_t lock;
    /* Deadlock detection */
    uint64_t        wait_for[MAX_TRANSACTIONS]; /* txn_id -> waiting_for_txn_id */
} LockManager;

/* ============================================================
 * REPLICATION
 * ============================================================ */

typedef enum {
    REPL_STATE_INIT,
    REPL_STATE_STREAMING,
    REPL_STATE_CAUGHT_UP,
    REPL_STATE_DISCONNECTED,
    REPL_STATE_ERROR,
} ReplState;

typedef enum {
    REPL_ROLE_PRIMARY,
    REPL_ROLE_REPLICA,
    REPL_ROLE_STANDALONE,
} ReplRole;

typedef struct {
    int            socket_fd;
    char           host[256];
    uint16_t       port;
    ReplState      state;
    uint64_t       last_lsn;
    uint64_t       last_heartbeat;
    pthread_t      thread;
} ReplicaInfo;

typedef struct {
    ReplRole        role;
    ReplicaInfo     replicas[MAX_REPLICAS];
    uint32_t        num_replicas;
    char            primary_host[256];
    uint16_t        primary_port;
    uint64_t        last_applied_lsn;
    WALManager     *wal;
    pthread_mutex_t lock;
    bool            sync_replication;  /* Senkron mu asenkron mu */
} ReplicationManager;

/* ============================================================
 * SERVER & BAĞLANTI
 * ============================================================ */

typedef struct {
    int             socket_fd;
    uint32_t        conn_id;
    pthread_t       thread;
    Transaction    *current_txn;
    bool            active;
    char            client_addr[64];
    uint16_t        client_port;
    time_t          connected_at;
    uint64_t        queries_executed;
} Connection;

typedef struct {
    int             listen_fd;
    uint16_t        port;
    Connection      connections[MAX_CONNECTIONS];
    uint32_t        num_connections;
    pthread_t       accept_thread;
    bool            running;
    pthread_mutex_t lock;
} Server;

/* ============================================================
 * ANA VERİTABANI YAPISI
 * ============================================================ */

typedef struct {
    char              name[256];
    char              data_dir[512];
    Pager            *pager;
    TableSchema       tables[MAX_TABLES];
    uint32_t          num_tables;
    BTree            *indexes[MAX_TABLES];  /* Her tablo için PK index */
    TxnManager       *txn_mgr;
    WALManager       *wal_mgr;
    LockManager      *lock_mgr;
    ReplicationManager *repl_mgr;
    Server           *server;
    bool              is_open;
    pthread_rwlock_t  schema_lock;
    
    /* İstatistikler */
    uint64_t          total_queries;
    uint64_t          total_inserts;
    uint64_t          total_updates;
    uint64_t          total_deletes;
    uint64_t          total_selects;
    time_t            started_at;
} Database;

/* ============================================================
 * FONKSİYON PROTOTIPLERI
 * ============================================================ */

/* Pager */
ZeusStatus pager_open(Pager **pager, const char *filepath);
ZeusStatus pager_close(Pager *pager);
ZeusStatus pager_read_page(Pager *pager, uint32_t page_num, Page *page);
ZeusStatus pager_write_page(Pager *pager, uint32_t page_num, const Page *page);
ZeusStatus pager_allocate_page(Pager *pager, uint32_t *page_num);
ZeusStatus pager_free_page(Pager *pager, uint32_t page_num);
ZeusStatus pager_flush(Pager *pager);
uint32_t   pager_checksum(const uint8_t *data, uint32_t len);

/* B+Tree */
ZeusStatus btree_create(BTree **tree, Pager *pager);
ZeusStatus btree_destroy(BTree *tree);
ZeusStatus btree_insert(BTree *tree, uint32_t key, const uint8_t *data, uint16_t data_size);
ZeusStatus btree_search(BTree *tree, uint32_t key, uint8_t *data, uint16_t *data_size);
ZeusStatus btree_delete(BTree *tree, uint32_t key);
ZeusStatus btree_update(BTree *tree, uint32_t key, const uint8_t *data, uint16_t data_size);
ZeusStatus btree_scan(BTree *tree, uint32_t start_key, uint32_t end_key,
                       void (*callback)(uint32_t key, const uint8_t *data, uint16_t size, void *ctx),
                       void *ctx);

/* Table Manager */
ZeusStatus table_create(Database *db, const ASTCreateTable *stmt);
ZeusStatus table_drop(Database *db, const char *table_name);
TableSchema* table_find(Database *db, const char *table_name);
ZeusStatus table_insert_row(Database *db, TableSchema *schema, Row *row);
ZeusStatus table_delete_row(Database *db, TableSchema *schema, uint32_t rowid);
ZeusStatus table_update_row(Database *db, TableSchema *schema, uint32_t rowid, Row *row);
ZeusStatus table_scan(Database *db, TableSchema *schema,
                      bool (*filter)(const Row *row, void *ctx),
                      void *ctx, ResultSet *result);

/* SQL Tokenizer */
ZeusStatus tokenize(const char *sql, Token *tokens, uint32_t *num_tokens);

/* SQL Parser */
ZeusStatus parse(const Token *tokens, uint32_t num_tokens, ASTNode *ast);

/* Query Executor */
ZeusStatus execute(Database *db, const ASTNode *ast, ResultSet *result);
ZeusStatus execute_sql(Database *db, const char *sql, ResultSet *result);

/* Result Set */
ResultSet* resultset_create(void);
void       resultset_free(ResultSet *rs);
ZeusStatus resultset_add_row(ResultSet *rs, const Row *row);
void       resultset_print(const ResultSet *rs);

/* WAL Manager */
ZeusStatus wal_open(WALManager **wal, const char *dirpath);
ZeusStatus wal_close(WALManager *wal);
ZeusStatus wal_write(WALManager *wal, const WALRecord *record, const uint8_t *data);
ZeusStatus wal_flush(WALManager *wal);
ZeusStatus wal_checkpoint(WALManager *wal, Pager *pager);
ZeusStatus wal_recover(WALManager *wal, Database *db);

/* Transaction Manager */
ZeusStatus txn_mgr_init(TxnManager **mgr, WALManager *wal);
ZeusStatus txn_mgr_destroy(TxnManager *mgr);
ZeusStatus txn_begin(TxnManager *mgr, Transaction **txn);
ZeusStatus txn_commit(TxnManager *mgr, Transaction *txn);
ZeusStatus txn_rollback(TxnManager *mgr, Transaction *txn);

/* Lock Manager */
ZeusStatus lock_mgr_init(LockManager **mgr);
ZeusStatus lock_mgr_destroy(LockManager *mgr);
ZeusStatus lock_acquire(LockManager *mgr, uint64_t txn_id, uint32_t resource_id,
                         LockMode mode, LockGranularity granularity);
ZeusStatus lock_release(LockManager *mgr, uint64_t txn_id, uint32_t resource_id);
ZeusStatus lock_release_all(LockManager *mgr, uint64_t txn_id);
bool       lock_detect_deadlock(LockManager *mgr, uint64_t txn_id);

/* Replication Manager */
ZeusStatus repl_init(ReplicationManager **mgr, WALManager *wal, ReplRole role);
ZeusStatus repl_destroy(ReplicationManager *mgr);
ZeusStatus repl_add_replica(ReplicationManager *mgr, const char *host, uint16_t port);
ZeusStatus repl_remove_replica(ReplicationManager *mgr, const char *host, uint16_t port);
ZeusStatus repl_start(ReplicationManager *mgr);
ZeusStatus repl_stop(ReplicationManager *mgr);
ZeusStatus repl_send_wal(ReplicationManager *mgr, const WALRecord *record, const uint8_t *data);

/* Server */
ZeusStatus server_init(Server **srv, uint16_t port);
ZeusStatus server_start(Server *srv, Database *db);
ZeusStatus server_stop(Server *srv);

/* Database */
ZeusStatus db_open(Database **db, const char *name, const char *data_dir);
ZeusStatus db_close(Database *db);

/* CLI */
void       cli_run(Database *db);

/* Utility */
const char* zeus_status_str(ZeusStatus status);
void        zeus_log(const char *level, const char *fmt, ...);
uint64_t    zeus_timestamp_us(void);
void        zeus_value_free(ZeusValue *val);
ZeusValue   zeus_value_int(int64_t v);
ZeusValue   zeus_value_float(double v);
ZeusValue   zeus_value_text(const char *s);
ZeusValue   zeus_value_bool(bool v);
ZeusValue   zeus_value_null(void);
int         zeus_value_compare(const ZeusValue *a, const ZeusValue *b);
ZeusStatus  zeus_value_copy(ZeusValue *dst, const ZeusValue *src);

/* Row serialization */
ZeusStatus row_serialize(const Row *row, const TableSchema *schema,
                          uint8_t *buffer, uint32_t *size);
ZeusStatus row_deserialize(const uint8_t *buffer, uint32_t size,
                            const TableSchema *schema, Row *row);

#endif /* ZEUSDB_H */
