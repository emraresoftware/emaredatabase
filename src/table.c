/*
 * ZeusDB - Table Manager & Row Serialization
 *
 * Tablo CRUD işlemleri ve satır serialize/deserialize.
 */

#include "../include/zeusdb.h"
#include <ctype.h>

/* ============================================================
 * UTILITY FONKSİYONLAR
 * ============================================================ */

const char* zeus_status_str(ZeusStatus status) {
    switch (status) {
        case ZEUS_OK:               return "OK";
        case ZEUS_ERROR:            return "Genel hata";
        case ZEUS_ERROR_IO:         return "I/O hatası";
        case ZEUS_ERROR_CORRUPT:    return "Bozuk veri";
        case ZEUS_ERROR_FULL:       return "Veritabanı dolu";
        case ZEUS_ERROR_NOT_FOUND:  return "Bulunamadı";
        case ZEUS_ERROR_EXISTS:     return "Zaten mevcut";
        case ZEUS_ERROR_CONSTRAINT: return "Kısıtlama ihlali";
        case ZEUS_ERROR_TYPE:       return "Tip uyumsuzluğu";
        case ZEUS_ERROR_SYNTAX:     return "Sözdizimi hatası";
        case ZEUS_ERROR_INTERNAL:   return "Dahili hata";
        case ZEUS_ERROR_TXN_ABORT:  return "Transaction iptal";
        case ZEUS_ERROR_DEADLOCK:   return "Deadlock tespit edildi";
        case ZEUS_ERROR_TIMEOUT:    return "Zaman aşımı";
        case ZEUS_ERROR_READONLY:   return "Salt okunur";
        case ZEUS_ERROR_NOMEM:      return "Bellek yetersiz";
        case ZEUS_ERROR_BUSY:       return "Meşgul";
        case ZEUS_ERROR_OVERFLOW:   return "Taşma";
        case ZEUS_ERROR_PERMISSION: return "Yetki hatası";
        default: return "Bilinmeyen hata";
    }
}

void zeus_log(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(stderr, "[%s] [%s] ", timebuf, level);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

uint64_t zeus_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* ============================================================
 * VALUE FONKSİYONLARI
 * ============================================================ */

void zeus_value_free(ZeusValue *val) {
    if (!val) return;
    if (val->type == ZEUS_TYPE_TEXT && val->str_val.data) {
        free(val->str_val.data);
        val->str_val.data = NULL;
    }
    if (val->type == ZEUS_TYPE_BLOB && val->blob_val.data) {
        free(val->blob_val.data);
        val->blob_val.data = NULL;
    }
}

ZeusValue zeus_value_int(int64_t v) {
    ZeusValue val = {0};
    val.type = ZEUS_TYPE_INT;
    val.int_val = v;
    val.is_null = false;
    return val;
}

ZeusValue zeus_value_float(double v) {
    ZeusValue val = {0};
    val.type = ZEUS_TYPE_FLOAT;
    val.float_val = v;
    val.is_null = false;
    return val;
}

ZeusValue zeus_value_text(const char *s) {
    ZeusValue val = {0};
    val.type = ZEUS_TYPE_TEXT;
    if (s) {
        val.str_val.data = strdup(s);
        val.str_val.len = (uint32_t)strlen(s);
    }
    val.is_null = (s == NULL);
    return val;
}

ZeusValue zeus_value_bool(bool v) {
    ZeusValue val = {0};
    val.type = ZEUS_TYPE_BOOL;
    val.bool_val = v;
    val.is_null = false;
    return val;
}

ZeusValue zeus_value_null(void) {
    ZeusValue val = {0};
    val.type = ZEUS_TYPE_NULL;
    val.is_null = true;
    return val;
}

int zeus_value_compare(const ZeusValue *a, const ZeusValue *b) {
    if (a->is_null && b->is_null) return 0;
    if (a->is_null) return -1;
    if (b->is_null) return 1;

    if (a->type != b->type) {
        /* Tip dönüşümü dene */
        if (a->type == ZEUS_TYPE_INT && b->type == ZEUS_TYPE_FLOAT) {
            double av = (double)a->int_val;
            if (av < b->float_val) return -1;
            if (av > b->float_val) return 1;
            return 0;
        }
        if (a->type == ZEUS_TYPE_FLOAT && b->type == ZEUS_TYPE_INT) {
            double bv = (double)b->int_val;
            if (a->float_val < bv) return -1;
            if (a->float_val > bv) return 1;
            return 0;
        }
        return (int)a->type - (int)b->type;
    }

    switch (a->type) {
        case ZEUS_TYPE_INT:
            if (a->int_val < b->int_val) return -1;
            if (a->int_val > b->int_val) return 1;
            return 0;
        case ZEUS_TYPE_FLOAT:
            if (a->float_val < b->float_val) return -1;
            if (a->float_val > b->float_val) return 1;
            return 0;
        case ZEUS_TYPE_TEXT:
            if (a->str_val.data && b->str_val.data)
                return strcmp(a->str_val.data, b->str_val.data);
            return 0;
        case ZEUS_TYPE_BOOL:
            return (int)a->bool_val - (int)b->bool_val;
        case ZEUS_TYPE_TIMESTAMP:
            if (a->timestamp_val < b->timestamp_val) return -1;
            if (a->timestamp_val > b->timestamp_val) return 1;
            return 0;
        default:
            return 0;
    }
}

ZeusStatus zeus_value_copy(ZeusValue *dst, const ZeusValue *src) {
    if (!dst || !src) return ZEUS_ERROR;
    *dst = *src;
    if (src->type == ZEUS_TYPE_TEXT && src->str_val.data) {
        dst->str_val.data = strdup(src->str_val.data);
        if (!dst->str_val.data) return ZEUS_ERROR_NOMEM;
    }
    if (src->type == ZEUS_TYPE_BLOB && src->blob_val.data) {
        dst->blob_val.data = malloc(src->blob_val.len);
        if (!dst->blob_val.data) return ZEUS_ERROR_NOMEM;
        memcpy(dst->blob_val.data, src->blob_val.data, src->blob_val.len);
    }
    return ZEUS_OK;
}

/* ============================================================
 * ROW SERIALIZATION
 * ============================================================ */

ZeusStatus row_serialize(const Row *row, const TableSchema *schema,
                          uint8_t *buffer, uint32_t *size) {
    if (!row || !schema || !buffer || !size) return ZEUS_ERROR;

    uint32_t offset = 0;

    /* ROWID */
    memcpy(buffer + offset, &row->rowid, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    /* Null bitmap (1 bit per column) */
    uint32_t bitmap_size = (schema->num_columns + 7) / 8;
    uint8_t *nullmap = buffer + offset;
    memset(nullmap, 0, bitmap_size);

    for (uint32_t i = 0; i < schema->num_columns && i < row->num_values; i++) {
        if (row->values[i].is_null) {
            nullmap[i / 8] |= (1 << (i % 8));
        }
    }
    offset += bitmap_size;

    /* Her sütunun verisini yaz */
    for (uint32_t i = 0; i < schema->num_columns && i < row->num_values; i++) {
        if (row->values[i].is_null) continue;

        const ZeusValue *val = &row->values[i];
        switch (val->type) {
            case ZEUS_TYPE_INT:
                memcpy(buffer + offset, &val->int_val, sizeof(int64_t));
                offset += sizeof(int64_t);
                break;
            case ZEUS_TYPE_FLOAT:
                memcpy(buffer + offset, &val->float_val, sizeof(double));
                offset += sizeof(double);
                break;
            case ZEUS_TYPE_TEXT: {
                uint32_t len = val->str_val.len;
                memcpy(buffer + offset, &len, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                if (len > 0 && val->str_val.data) {
                    memcpy(buffer + offset, val->str_val.data, len);
                    offset += len;
                }
                break;
            }
            case ZEUS_TYPE_BOOL:
                buffer[offset++] = val->bool_val ? 1 : 0;
                break;
            case ZEUS_TYPE_TIMESTAMP:
                memcpy(buffer + offset, &val->timestamp_val, sizeof(int64_t));
                offset += sizeof(int64_t);
                break;
            default:
                break;
        }
    }

    *size = offset;
    return ZEUS_OK;
}

ZeusStatus row_deserialize(const uint8_t *buffer, uint32_t size,
                            const TableSchema *schema, Row *row) {
    if (!buffer || !schema || !row) return ZEUS_ERROR;

    memset(row, 0, sizeof(Row));
    uint32_t offset = 0;

    /* ROWID */
    memcpy(&row->rowid, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    /* Null bitmap */
    uint32_t bitmap_size = (schema->num_columns + 7) / 8;
    const uint8_t *nullmap = buffer + offset;
    offset += bitmap_size;

    row->num_values = schema->num_columns;

    for (uint32_t i = 0; i < schema->num_columns; i++) {
        bool is_null = (nullmap[i / 8] >> (i % 8)) & 1;
        if (is_null) {
            row->values[i] = zeus_value_null();
            continue;
        }

        row->values[i].is_null = false;
        row->values[i].type = schema->columns[i].type;

        switch (schema->columns[i].type) {
            case ZEUS_TYPE_INT:
                memcpy(&row->values[i].int_val, buffer + offset, sizeof(int64_t));
                offset += sizeof(int64_t);
                break;
            case ZEUS_TYPE_FLOAT:
                memcpy(&row->values[i].float_val, buffer + offset, sizeof(double));
                offset += sizeof(double);
                break;
            case ZEUS_TYPE_TEXT: {
                uint32_t len;
                memcpy(&len, buffer + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                row->values[i].str_val.data = malloc(len + 1);
                if (row->values[i].str_val.data && len > 0) {
                    memcpy(row->values[i].str_val.data, buffer + offset, len);
                    row->values[i].str_val.data[len] = '\0';
                } else if (row->values[i].str_val.data) {
                    row->values[i].str_val.data[0] = '\0';
                }
                row->values[i].str_val.len = len;
                offset += len;
                break;
            }
            case ZEUS_TYPE_BOOL:
                row->values[i].bool_val = buffer[offset++] ? true : false;
                break;
            case ZEUS_TYPE_TIMESTAMP:
                memcpy(&row->values[i].timestamp_val, buffer + offset, sizeof(int64_t));
                offset += sizeof(int64_t);
                break;
            default:
                break;
        }
    }

    return ZEUS_OK;
}

/* ============================================================
 * RESULT SET
 * ============================================================ */

ResultSet* resultset_create(void) {
    ResultSet *rs = calloc(1, sizeof(ResultSet));
    if (!rs) return NULL;
    rs->capacity = 64;
    rs->rows = calloc(rs->capacity, sizeof(Row));
    if (!rs->rows) {
        free(rs);
        return NULL;
    }
    return rs;
}

void resultset_free(ResultSet *rs) {
    if (!rs) return;
    if (rs->rows) {
        for (uint32_t i = 0; i < rs->num_rows; i++) {
            for (uint32_t j = 0; j < rs->rows[i].num_values; j++) {
                zeus_value_free(&rs->rows[i].values[j]);
            }
        }
        free(rs->rows);
    }
    free(rs);
}

ZeusStatus resultset_add_row(ResultSet *rs, const Row *row) {
    if (!rs || !row) return ZEUS_ERROR;

    /* Kapasite kontrolü */
    if (rs->num_rows >= rs->capacity) {
        uint32_t new_cap = rs->capacity * 2;
        Row *new_rows = realloc(rs->rows, new_cap * sizeof(Row));
        if (!new_rows) return ZEUS_ERROR_NOMEM;
        rs->rows = new_rows;
        rs->capacity = new_cap;
    }

    /* Row'u kopyala */
    Row *dst = &rs->rows[rs->num_rows];
    dst->rowid = row->rowid;
    dst->num_values = row->num_values;

    for (uint32_t i = 0; i < row->num_values; i++) {
        zeus_value_copy(&dst->values[i], &row->values[i]);
    }

    rs->num_rows++;
    return ZEUS_OK;
}

/* ============================================================
 * RESULT SET YAZDIRMA (CLI için)
 * ============================================================ */

void resultset_print(const ResultSet *rs) {
    if (!rs) return;

    if (rs->num_rows == 0 && rs->num_cols == 0) {
        printf("Boş sonuç kümesi.\n");
        return;
    }

    /* Sütun genişliklerini hesapla */
    uint32_t widths[MAX_COLUMNS] = {0};

    for (uint32_t c = 0; c < rs->num_cols; c++) {
        widths[c] = (uint32_t)strlen(rs->col_names[c]);
        if (widths[c] < 4) widths[c] = 4;
    }

    for (uint32_t r = 0; r < rs->num_rows; r++) {
        for (uint32_t c = 0; c < rs->num_cols && c < rs->rows[r].num_values; c++) {
            uint32_t len = 0;
            const ZeusValue *val = &rs->rows[r].values[c];
            if (val->is_null) {
                len = 4; /* NULL */
            } else {
                switch (val->type) {
                    case ZEUS_TYPE_INT: {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%lld", (long long)val->int_val);
                        len = (uint32_t)strlen(buf);
                        break;
                    }
                    case ZEUS_TYPE_FLOAT: {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.4f", val->float_val);
                        len = (uint32_t)strlen(buf);
                        break;
                    }
                    case ZEUS_TYPE_TEXT:
                        len = val->str_val.len;
                        break;
                    case ZEUS_TYPE_BOOL:
                        len = val->bool_val ? 4 : 5;
                        break;
                    default:
                        len = 4;
                        break;
                }
            }
            if (len > widths[c]) widths[c] = len;
            if (widths[c] > 50) widths[c] = 50; /* Max genişlik */
        }
    }

    /* Üst çizgi */
    printf("┌");
    for (uint32_t c = 0; c < rs->num_cols; c++) {
        for (uint32_t j = 0; j < widths[c] + 2; j++) printf("─");
        printf(c < rs->num_cols - 1 ? "┬" : "┐");
    }
    printf("\n");

    /* Header */
    printf("│");
    for (uint32_t c = 0; c < rs->num_cols; c++) {
        printf(" %-*s │", widths[c], rs->col_names[c]);
    }
    printf("\n");

    /* Ayırıcı */
    printf("├");
    for (uint32_t c = 0; c < rs->num_cols; c++) {
        for (uint32_t j = 0; j < widths[c] + 2; j++) printf("─");
        printf(c < rs->num_cols - 1 ? "┼" : "┤");
    }
    printf("\n");

    /* Veriler */
    for (uint32_t r = 0; r < rs->num_rows; r++) {
        printf("│");
        for (uint32_t c = 0; c < rs->num_cols; c++) {
            if (c < rs->rows[r].num_values) {
                const ZeusValue *val = &rs->rows[r].values[c];
                if (val->is_null) {
                    printf(" %-*s │", widths[c], "NULL");
                } else {
                    switch (val->type) {
                        case ZEUS_TYPE_INT:
                            printf(" %-*lld │", widths[c], (long long)val->int_val);
                            break;
                        case ZEUS_TYPE_FLOAT:
                            printf(" %-*.4f │", widths[c], val->float_val);
                            break;
                        case ZEUS_TYPE_TEXT:
                            printf(" %-*.*s │", widths[c], widths[c],
                                   val->str_val.data ? val->str_val.data : "");
                            break;
                        case ZEUS_TYPE_BOOL:
                            printf(" %-*s │", widths[c], val->bool_val ? "true" : "false");
                            break;
                        case ZEUS_TYPE_TIMESTAMP:
                            printf(" %-*lld │", widths[c], (long long)val->timestamp_val);
                            break;
                        default:
                            printf(" %-*s │", widths[c], "?");
                            break;
                    }
                }
            } else {
                printf(" %-*s │", widths[c], "");
            }
        }
        printf("\n");
    }

    /* Alt çizgi */
    printf("└");
    for (uint32_t c = 0; c < rs->num_cols; c++) {
        for (uint32_t j = 0; j < widths[c] + 2; j++) printf("─");
        printf(c < rs->num_cols - 1 ? "┴" : "┘");
    }
    printf("\n");

    printf("%u satır (%llu µs)\n", rs->num_rows, (unsigned long long)rs->exec_time_us);
}

/* ============================================================
 * TABLE MANAGER
 * ============================================================ */

TableSchema* table_find(Database *db, const char *table_name) {
    if (!db || !table_name) return NULL;

    for (uint32_t i = 0; i < db->num_tables; i++) {
        if (strcasecmp(db->tables[i].name, table_name) == 0) {
            return &db->tables[i];
        }
    }
    return NULL;
}

ZeusStatus table_create(Database *db, const ASTCreateTable *stmt) {
    if (!db || !stmt) return ZEUS_ERROR;

    /* Tablo zaten var mı? */
    if (table_find(db, stmt->table_name)) {
        if (stmt->if_not_exists) return ZEUS_OK;
        zeus_log("ERROR", "Tablo zaten mevcut: %s", stmt->table_name);
        return ZEUS_ERROR_EXISTS;
    }

    if (db->num_tables >= MAX_TABLES) {
        zeus_log("ERROR", "Maksimum tablo sayısı aşıldı");
        return ZEUS_ERROR_FULL;
    }

    /* Yeni tablo şeması */
    TableSchema *schema = &db->tables[db->num_tables];
    memset(schema, 0, sizeof(TableSchema));

    strncpy(schema->name, stmt->table_name, MAX_TABLE_NAME - 1);
    schema->num_columns = stmt->num_columns;
    schema->next_rowid = 1;
    schema->has_primary_key = false;

    for (uint32_t i = 0; i < stmt->num_columns; i++) {
        memcpy(&schema->columns[i], &stmt->columns[i], sizeof(ColumnDef));
        if (stmt->columns[i].flags & COL_FLAG_PRIMARY_KEY) {
            schema->primary_key_col = i;
            schema->has_primary_key = true;
        }
    }

    /* B+Tree index oluştur */
    BTree *tree;
    ZeusStatus rc = btree_create(&tree, db->pager);
    if (rc != ZEUS_OK) return rc;

    schema->root_page = tree->root_page;
    db->indexes[db->num_tables] = tree;

    /* Schema bilgisini diske yaz */
    uint32_t schema_page;
    rc = pager_allocate_page(db->pager, &schema_page);
    if (rc != ZEUS_OK) return rc;

    Page page;
    memset(&page, 0, sizeof(Page));
    page.header.page_num = schema_page;
    page.header.type = PAGE_TYPE_TABLE_SCHEMA;

    /* Schema'yı serialize et */
    memcpy(page.data, schema, sizeof(TableSchema));
    pager_write_page(db->pager, schema_page, &page);

    db->num_tables++;

    zeus_log("INFO", "Tablo oluşturuldu: %s (%u sütun)", schema->name, schema->num_columns);
    return ZEUS_OK;
}

ZeusStatus table_drop(Database *db, const char *table_name) {
    if (!db || !table_name) return ZEUS_ERROR;

    int idx = -1;
    for (uint32_t i = 0; i < db->num_tables; i++) {
        if (strcasecmp(db->tables[i].name, table_name) == 0) {
            idx = (int)i;
            break;
        }
    }

    if (idx < 0) {
        zeus_log("ERROR", "Tablo bulunamadı: %s", table_name);
        return ZEUS_ERROR_NOT_FOUND;
    }

    /* B+Tree'yi temizle */
    if (db->indexes[idx]) {
        btree_destroy(db->indexes[idx]);
        db->indexes[idx] = NULL;
    }

    /* Tabloyu diziden çıkar (sona kaydır) */
    for (uint32_t i = idx; i < db->num_tables - 1; i++) {
        db->tables[i] = db->tables[i + 1];
        db->indexes[i] = db->indexes[i + 1];
    }
    db->num_tables--;

    zeus_log("INFO", "Tablo silindi: %s", table_name);
    return ZEUS_OK;
}

ZeusStatus table_insert_row(Database *db, TableSchema *schema, Row *row) {
    if (!db || !schema || !row) return ZEUS_ERROR;

    /* Auto increment */
    if (row->rowid == 0) {
        row->rowid = schema->next_rowid++;
    }

    /* Satırı serialize et */
    uint8_t buffer[MAX_ROW_SIZE];
    uint32_t size;
    ZeusStatus rc = row_serialize(row, schema, buffer, &size);
    if (rc != ZEUS_OK) return rc;

    /* B+Tree'ye ekle */
    int table_idx = -1;
    for (uint32_t i = 0; i < db->num_tables; i++) {
        if (&db->tables[i] == schema) {
            table_idx = (int)i;
            break;
        }
    }

    if (table_idx < 0 || !db->indexes[table_idx]) {
        return ZEUS_ERROR_INTERNAL;
    }

    rc = btree_insert(db->indexes[table_idx], row->rowid, buffer, (uint16_t)size);
    if (rc != ZEUS_OK) return rc;

    schema->row_count++;

    /* Veriyi ayrıca data page'e yaz */
    uint32_t data_page;
    rc = pager_allocate_page(db->pager, &data_page);
    if (rc != ZEUS_OK) return rc;

    Page page;
    memset(&page, 0, sizeof(Page));
    page.header.page_num = data_page;
    page.header.type = PAGE_TYPE_BTREE_LEAF;
    page.header.num_cells = 1;

    /* rowid + size + data */
    uint32_t off = 0;
    memcpy(page.data + off, &row->rowid, sizeof(uint32_t)); off += sizeof(uint32_t);
    memcpy(page.data + off, &size, sizeof(uint32_t)); off += sizeof(uint32_t);
    memcpy(page.data + off, buffer, size);

    pager_write_page(db->pager, data_page, &page);

    return ZEUS_OK;
}

ZeusStatus table_delete_row(Database *db, TableSchema *schema, uint32_t rowid) {
    if (!db || !schema) return ZEUS_ERROR;

    int table_idx = -1;
    for (uint32_t i = 0; i < db->num_tables; i++) {
        if (&db->tables[i] == schema) {
            table_idx = (int)i;
            break;
        }
    }

    if (table_idx < 0 || !db->indexes[table_idx]) return ZEUS_ERROR_INTERNAL;

    ZeusStatus rc = btree_delete(db->indexes[table_idx], rowid);
    if (rc == ZEUS_OK) {
        schema->row_count--;
    }
    return rc;
}

/* ============================================================
 * TABLE SCAN - Tüm satırları okuyup filtre uygula
 * ============================================================ */

/* In-memory row storage for simplicity */
typedef struct {
    Row     *rows;
    uint32_t count;
    uint32_t capacity;
} RowStore;

static RowStore* rowstore_global = NULL;

/* B+Tree tüm leaf'leri tarayarak veri okuma */
ZeusStatus table_scan(Database *db, TableSchema *schema,
                      bool (*filter)(const Row *row, void *ctx),
                      void *ctx, ResultSet *result) {
    if (!db || !schema || !result) return ZEUS_ERROR;

    /* Tüm data page'leri tara */
    for (uint32_t p = 1; p < db->pager->num_pages; p++) {
        Page page;
        ZeusStatus rc = pager_read_page(db->pager, p, &page);
        if (rc != ZEUS_OK) continue;

        if (page.header.type != PAGE_TYPE_BTREE_LEAF) continue;
        if (page.header.num_cells == 0) continue;

        /* Sayfadaki her cell'i oku */
        uint32_t off = 0;
        for (uint32_t c = 0; c < page.header.num_cells; c++) {
            uint32_t rowid;
            uint32_t data_size;
            memcpy(&rowid, page.data + off, sizeof(uint32_t)); off += sizeof(uint32_t);
            memcpy(&data_size, page.data + off, sizeof(uint32_t)); off += sizeof(uint32_t);

            if (data_size > 0 && data_size < MAX_ROW_SIZE) {
                Row row;
                rc = row_deserialize(page.data + off, data_size, schema, &row);
                if (rc == ZEUS_OK) {
                    if (!filter || filter(&row, ctx)) {
                        resultset_add_row(result, &row);
                    }
                    /* Free text values */
                    for (uint32_t v = 0; v < row.num_values; v++) {
                        zeus_value_free(&row.values[v]);
                    }
                }
                off += data_size;
            }
        }
    }

    return ZEUS_OK;
}
