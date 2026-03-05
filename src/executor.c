/*
 * ZeusDB - Query Executor
 *
 * AST'yi alıp veritabanı üzerinde çalıştırır.
 * Desteklenen operasyonlar:
 *   - CREATE TABLE / DROP TABLE
 *   - INSERT, SELECT, UPDATE, DELETE
 *   - BEGIN, COMMIT, ROLLBACK
 *   - SHOW TABLES, DESCRIBE
 *   - WHERE filtreleme (=, !=, <, >, <=, >=, LIKE, IS NULL)
 *   - ORDER BY, LIMIT, OFFSET
 *   - Aggregate fonksiyonları (COUNT, SUM, AVG, MIN, MAX)
 */

#include "../include/zeusdb.h"
#include <ctype.h>

/* ============================================================
 * WHERE FİLTRE DEĞERLENDİRME
 * ============================================================ */

/* Basit LIKE pattern matching (% ve _ desteği) */
static bool like_match(const char *str, const char *pattern) {
    if (!str || !pattern) return false;

    while (*pattern) {
        if (*pattern == '%') {
            pattern++;
            if (!*pattern) return true;
            while (*str) {
                if (like_match(str, pattern)) return true;
                str++;
            }
            return false;
        } else if (*pattern == '_') {
            if (!*str) return false;
            str++;
            pattern++;
        } else {
            if (tolower((unsigned char)*str) != tolower((unsigned char)*pattern))
                return false;
            str++;
            pattern++;
        }
    }
    return *str == '\0';
}

static bool evaluate_condition(const Row *row, const TableSchema *schema,
                                const WhereClause *wc) {
    /* Sütunu bul */
    int col_idx = -1;
    for (uint32_t i = 0; i < schema->num_columns; i++) {
        if (strcasecmp(schema->columns[i].name, wc->column) == 0) {
            col_idx = (int)i;
            break;
        }
    }

    if (col_idx < 0 || (uint32_t)col_idx >= row->num_values) return false;

    const ZeusValue *val = &row->values[col_idx];

    /* IS NULL / IS NOT NULL */
    if (wc->op == CMP_IS_NULL) return val->is_null;
    if (wc->op == CMP_IS_NOT_NULL) return !val->is_null;

    if (val->is_null) return false;

    /* LIKE */
    if (wc->op == CMP_LIKE) {
        if (val->type != ZEUS_TYPE_TEXT || !val->str_val.data) return false;
        if (!wc->value.str_val.data) return false;
        return like_match(val->str_val.data, wc->value.str_val.data);
    }

    /* BETWEEN */
    if (wc->op == CMP_BETWEEN) {
        int cmp1 = zeus_value_compare(val, &wc->value);
        int cmp2 = zeus_value_compare(val, &wc->value2);
        return cmp1 >= 0 && cmp2 <= 0;
    }

    /* Karşılaştırma */
    int cmp = zeus_value_compare(val, &wc->value);

    switch (wc->op) {
        case CMP_EQ:  return cmp == 0;
        case CMP_NEQ: return cmp != 0;
        case CMP_LT:  return cmp < 0;
        case CMP_GT:  return cmp > 0;
        case CMP_LTE: return cmp <= 0;
        case CMP_GTE: return cmp >= 0;
        default:      return false;
    }
}

static bool evaluate_where(const Row *row, const TableSchema *schema,
                            const WhereClause *clauses, uint32_t num_clauses) {
    if (num_clauses == 0) return true;

    bool result = evaluate_condition(row, schema, &clauses[0]);

    for (uint32_t i = 1; i < num_clauses; i++) {
        bool next = evaluate_condition(row, schema, &clauses[i]);

        if (clauses[i - 1].logic == LOGIC_AND) {
            result = result && next;
        } else {
            result = result || next;
        }
    }

    return result;
}

/* ============================================================
 * EXECUTE: CREATE TABLE
 * ============================================================ */

static ZeusStatus exec_create_table(Database *db, const ASTCreateTable *stmt, ResultSet *rs) {
    ZeusStatus rc = table_create(db, stmt);
    if (rc == ZEUS_OK) {
        rs->rows_affected = 0;
        rs->num_cols = 1;
        strncpy(rs->col_names[0], "Result", MAX_COLUMN_NAME - 1);
        rs->col_types[0] = ZEUS_TYPE_TEXT;

        Row row = {0};
        row.num_values = 1;
        row.values[0] = zeus_value_text("Tablo oluşturuldu");
        resultset_add_row(rs, &row);
        zeus_value_free(&row.values[0]);
    }
    return rc;
}

/* ============================================================
 * EXECUTE: DROP TABLE
 * ============================================================ */

static ZeusStatus exec_drop_table(Database *db, const ASTDropTable *stmt, ResultSet *rs) {
    ZeusStatus rc = table_drop(db, stmt->table_name);
    if (rc == ZEUS_OK || (rc == ZEUS_ERROR_NOT_FOUND && stmt->if_exists)) {
        rs->rows_affected = 0;
        rs->num_cols = 1;
        strncpy(rs->col_names[0], "Result", MAX_COLUMN_NAME - 1);

        Row row = {0};
        row.num_values = 1;
        row.values[0] = zeus_value_text("Tablo silindi");
        resultset_add_row(rs, &row);
        zeus_value_free(&row.values[0]);
        return ZEUS_OK;
    }
    return rc;
}

/* ============================================================
 * EXECUTE: INSERT
 * ============================================================ */

static ZeusStatus exec_insert(Database *db, const ASTInsert *stmt, ResultSet *rs) {
    TableSchema *schema = table_find(db, stmt->table_name);
    if (!schema) {
        zeus_log("ERROR", "Tablo bulunamadı: %s", stmt->table_name);
        return ZEUS_ERROR_NOT_FOUND;
    }

    Row row = {0};
    row.rowid = 0; /* Auto */
    row.num_values = schema->num_columns;

    /* Değerleri sütunlara ata */
    if (stmt->has_columns && stmt->num_columns > 0) {
        /* INSERT INTO t (col1, col2) VALUES (v1, v2) */
        /* Önce tüm değerleri NULL yap */
        for (uint32_t i = 0; i < schema->num_columns; i++) {
            row.values[i] = zeus_value_null();
        }

        for (uint32_t i = 0; i < stmt->num_columns && i < stmt->num_values; i++) {
            /* Sütunu bul */
            for (uint32_t j = 0; j < schema->num_columns; j++) {
                if (strcasecmp(schema->columns[j].name, stmt->columns[i]) == 0) {
                    zeus_value_copy(&row.values[j], &stmt->values[i]);
                    break;
                }
            }
        }
    } else {
        /* INSERT INTO t VALUES (v1, v2, ...) */
        for (uint32_t i = 0; i < stmt->num_values && i < schema->num_columns; i++) {
            zeus_value_copy(&row.values[i], &stmt->values[i]);
        }
    }

    /* NOT NULL kontrolü */
    for (uint32_t i = 0; i < schema->num_columns; i++) {
        if ((schema->columns[i].flags & COL_FLAG_NOT_NULL) && row.values[i].is_null) {
            /* Auto increment PK ise sorun yok */
            if (schema->columns[i].flags & COL_FLAG_AUTO_INC) {
                row.values[i] = zeus_value_int(schema->next_rowid);
                continue;
            }
            zeus_log("ERROR", "NOT NULL kısıtlaması ihlali: %s", schema->columns[i].name);
            return ZEUS_ERROR_CONSTRAINT;
        }
    }

    /* PK değerini rowid olarak kullan */
    if (schema->has_primary_key) {
        ZeusValue *pk_val = &row.values[schema->primary_key_col];
        if (pk_val->type == ZEUS_TYPE_INT && !pk_val->is_null) {
            row.rowid = (uint32_t)pk_val->int_val;
        }
    }

    ZeusStatus rc = table_insert_row(db, schema, &row);

    if (rc == ZEUS_OK) {
        db->total_inserts++;
        rs->rows_affected = 1;
        rs->num_cols = 1;
        strncpy(rs->col_names[0], "Result", MAX_COLUMN_NAME - 1);

        Row result_row = {0};
        result_row.num_values = 1;
        char msg[128];
        snprintf(msg, sizeof(msg), "1 satır eklendi (rowid: %u)", row.rowid);
        result_row.values[0] = zeus_value_text(msg);
        resultset_add_row(rs, &result_row);
        zeus_value_free(&result_row.values[0]);
    }

    /* Temizle */
    for (uint32_t i = 0; i < row.num_values; i++) {
        zeus_value_free(&row.values[i]);
    }

    return rc;
}

/* ============================================================
 * EXECUTE: SELECT
 * ============================================================ */

/* Sorting comparison context */
typedef struct {
    const TableSchema *schema;
    const OrderByClause *order_by;
    uint32_t num_order_by;
} SortCtx;

static SortCtx *sort_ctx_global = NULL;

static int sort_compare(const void *a, const void *b) {
    const Row *ra = (const Row *)a;
    const Row *rb = (const Row *)b;

    if (!sort_ctx_global) return 0;

    for (uint32_t i = 0; i < sort_ctx_global->num_order_by; i++) {
        const OrderByClause *ob = &sort_ctx_global->order_by[i];

        int col_idx = -1;
        for (uint32_t j = 0; j < sort_ctx_global->schema->num_columns; j++) {
            if (strcasecmp(sort_ctx_global->schema->columns[j].name, ob->column) == 0) {
                col_idx = (int)j;
                break;
            }
        }

        if (col_idx < 0) continue;

        int cmp = zeus_value_compare(&ra->values[col_idx], &rb->values[col_idx]);
        if (cmp != 0) {
            return ob->descending ? -cmp : cmp;
        }
    }
    return 0;
}

static ZeusStatus exec_select(Database *db, const ASTSelect *stmt, ResultSet *rs) {
    TableSchema *schema = table_find(db, stmt->table_name);
    if (!schema) {
        zeus_log("ERROR", "Tablo bulunamadı: %s", stmt->table_name);
        return ZEUS_ERROR_NOT_FOUND;
    }

    db->total_selects++;

    /* Sonuç sütunlarını ayarla */
    bool select_all = (stmt->num_columns == 1 && stmt->columns[0].is_star);
    bool is_aggregate = false;

    if (select_all) {
        rs->num_cols = schema->num_columns;
        for (uint32_t i = 0; i < schema->num_columns; i++) {
            strncpy(rs->col_names[i], schema->columns[i].name, MAX_COLUMN_NAME - 1);
            rs->col_types[i] = schema->columns[i].type;
        }
    } else {
        rs->num_cols = stmt->num_columns;
        for (uint32_t i = 0; i < stmt->num_columns; i++) {
            if (stmt->columns[i].is_aggregate) {
                is_aggregate = true;
                char buf[MAX_COLUMN_NAME];
                const char *func_name = "?";
                switch (stmt->columns[i].aggregate_func) {
                    case TOK_COUNT: func_name = "COUNT"; break;
                    case TOK_SUM:   func_name = "SUM"; break;
                    case TOK_AVG:   func_name = "AVG"; break;
                    case TOK_MIN_KW: func_name = "MIN"; break;
                    case TOK_MAX_KW: func_name = "MAX"; break;
                    default: break;
                }
                snprintf(buf, sizeof(buf), "%s(%s)", func_name, stmt->columns[i].name);

                if (stmt->columns[i].alias[0]) {
                    strncpy(rs->col_names[i], stmt->columns[i].alias, MAX_COLUMN_NAME - 1);
                } else {
                    strncpy(rs->col_names[i], buf, MAX_COLUMN_NAME - 1);
                }
                rs->col_types[i] = ZEUS_TYPE_INT; /* Aggregate default */
            } else {
                if (stmt->columns[i].alias[0]) {
                    strncpy(rs->col_names[i], stmt->columns[i].alias, MAX_COLUMN_NAME - 1);
                } else {
                    strncpy(rs->col_names[i], stmt->columns[i].name, MAX_COLUMN_NAME - 1);
                }
                for (uint32_t j = 0; j < schema->num_columns; j++) {
                    if (strcasecmp(schema->columns[j].name, stmt->columns[i].name) == 0) {
                        rs->col_types[i] = schema->columns[j].type;
                        break;
                    }
                }
            }
        }
    }

    /* Filtreleyerek satırları topla */
    ResultSet *temp = resultset_create();
    if (!temp) return ZEUS_ERROR_NOMEM;
    temp->num_cols = schema->num_columns;

    /* Tüm data page'leri tara */
    for (uint32_t p = 1; p < db->pager->num_pages; p++) {
        Page page;
        ZeusStatus rc = pager_read_page(db->pager, p, &page);
        if (rc != ZEUS_OK) continue;
        if (page.header.type != PAGE_TYPE_BTREE_LEAF) continue;
        if (page.header.num_cells == 0) continue;

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
                    /* WHERE filtre */
                    bool passes = true;
                    if (stmt->has_where) {
                        passes = evaluate_where(&row, schema, stmt->where, stmt->num_where);
                    }

                    if (passes) {
                        resultset_add_row(temp, &row);
                    }

                    for (uint32_t v = 0; v < row.num_values; v++) {
                        zeus_value_free(&row.values[v]);
                    }
                }
                off += data_size;
            }
        }
    }

    /* Aggregate fonksiyonları */
    if (is_aggregate) {
        Row agg_row = {0};
        agg_row.num_values = stmt->num_columns;

        for (uint32_t i = 0; i < stmt->num_columns; i++) {
            if (!stmt->columns[i].is_aggregate) continue;

            int col_idx = -1;
            if (strcmp(stmt->columns[i].name, "*") != 0) {
                for (uint32_t j = 0; j < schema->num_columns; j++) {
                    if (strcasecmp(schema->columns[j].name, stmt->columns[i].name) == 0) {
                        col_idx = (int)j;
                        break;
                    }
                }
            }

            switch (stmt->columns[i].aggregate_func) {
                case TOK_COUNT:
                    agg_row.values[i] = zeus_value_int((int64_t)temp->num_rows);
                    break;

                case TOK_SUM: {
                    double sum = 0;
                    for (uint32_t r = 0; r < temp->num_rows; r++) {
                        if (col_idx >= 0 && !temp->rows[r].values[col_idx].is_null) {
                            if (temp->rows[r].values[col_idx].type == ZEUS_TYPE_INT)
                                sum += (double)temp->rows[r].values[col_idx].int_val;
                            else if (temp->rows[r].values[col_idx].type == ZEUS_TYPE_FLOAT)
                                sum += temp->rows[r].values[col_idx].float_val;
                        }
                    }
                    agg_row.values[i] = zeus_value_float(sum);
                    rs->col_types[i] = ZEUS_TYPE_FLOAT;
                    break;
                }

                case TOK_AVG: {
                    double sum = 0;
                    uint32_t count = 0;
                    for (uint32_t r = 0; r < temp->num_rows; r++) {
                        if (col_idx >= 0 && !temp->rows[r].values[col_idx].is_null) {
                            if (temp->rows[r].values[col_idx].type == ZEUS_TYPE_INT)
                                sum += (double)temp->rows[r].values[col_idx].int_val;
                            else if (temp->rows[r].values[col_idx].type == ZEUS_TYPE_FLOAT)
                                sum += temp->rows[r].values[col_idx].float_val;
                            count++;
                        }
                    }
                    agg_row.values[i] = zeus_value_float(count > 0 ? sum / count : 0);
                    rs->col_types[i] = ZEUS_TYPE_FLOAT;
                    break;
                }

                case TOK_MIN_KW: {
                    bool found = false;
                    ZeusValue min_val = {0};
                    for (uint32_t r = 0; r < temp->num_rows; r++) {
                        if (col_idx >= 0 && !temp->rows[r].values[col_idx].is_null) {
                            if (!found || zeus_value_compare(&temp->rows[r].values[col_idx], &min_val) < 0) {
                                zeus_value_free(&min_val);
                                zeus_value_copy(&min_val, &temp->rows[r].values[col_idx]);
                                found = true;
                            }
                        }
                    }
                    agg_row.values[i] = found ? min_val : zeus_value_null();
                    break;
                }

                case TOK_MAX_KW: {
                    bool found = false;
                    ZeusValue max_val = {0};
                    for (uint32_t r = 0; r < temp->num_rows; r++) {
                        if (col_idx >= 0 && !temp->rows[r].values[col_idx].is_null) {
                            if (!found || zeus_value_compare(&temp->rows[r].values[col_idx], &max_val) > 0) {
                                zeus_value_free(&max_val);
                                zeus_value_copy(&max_val, &temp->rows[r].values[col_idx]);
                                found = true;
                            }
                        }
                    }
                    agg_row.values[i] = found ? max_val : zeus_value_null();
                    break;
                }

                default:
                    agg_row.values[i] = zeus_value_null();
                    break;
            }
        }

        resultset_add_row(rs, &agg_row);
        for (uint32_t v = 0; v < agg_row.num_values; v++) {
            zeus_value_free(&agg_row.values[v]);
        }

        resultset_free(temp);
        return ZEUS_OK;
    }

    /* ORDER BY */
    if (stmt->has_order_by && temp->num_rows > 1) {
        SortCtx sctx = {
            .schema = schema,
            .order_by = stmt->order_by,
            .num_order_by = stmt->num_order_by
        };
        sort_ctx_global = &sctx;
        qsort(temp->rows, temp->num_rows, sizeof(Row), sort_compare);
        sort_ctx_global = NULL;
    }

    /* OFFSET + LIMIT */
    uint32_t start = 0;
    uint32_t end = temp->num_rows;

    if (stmt->has_offset && stmt->offset > 0) {
        start = (uint32_t)stmt->offset;
        if (start > temp->num_rows) start = temp->num_rows;
    }

    if (stmt->has_limit && stmt->limit >= 0) {
        uint32_t limit_end = start + (uint32_t)stmt->limit;
        if (limit_end < end) end = limit_end;
    }

    /* Sonuç satırlarını oluştur */
    for (uint32_t r = start; r < end; r++) {
        Row *src_row = &temp->rows[r];

        if (select_all) {
            resultset_add_row(rs, src_row);
        } else {
            /* Seçili sütunları çekip koy */
            Row proj_row = {0};
            proj_row.rowid = src_row->rowid;
            proj_row.num_values = stmt->num_columns;

            for (uint32_t c = 0; c < stmt->num_columns; c++) {
                int col_idx = -1;
                for (uint32_t j = 0; j < schema->num_columns; j++) {
                    if (strcasecmp(schema->columns[j].name, stmt->columns[c].name) == 0) {
                        col_idx = (int)j;
                        break;
                    }
                }

                if (col_idx >= 0 && (uint32_t)col_idx < src_row->num_values) {
                    zeus_value_copy(&proj_row.values[c], &src_row->values[col_idx]);
                } else {
                    proj_row.values[c] = zeus_value_null();
                }
            }

            resultset_add_row(rs, &proj_row);
            for (uint32_t v = 0; v < proj_row.num_values; v++) {
                zeus_value_free(&proj_row.values[v]);
            }
        }
    }

    resultset_free(temp);
    return ZEUS_OK;
}

/* ============================================================
 * EXECUTE: UPDATE
 * ============================================================ */

static ZeusStatus exec_update(Database *db, const ASTUpdate *stmt, ResultSet *rs) {
    TableSchema *schema = table_find(db, stmt->table_name);
    if (!schema) return ZEUS_ERROR_NOT_FOUND;

    db->total_updates++;
    uint32_t updated = 0;

    /* Tüm data page'leri tara */
    for (uint32_t p = 1; p < db->pager->num_pages; p++) {
        Page page;
        ZeusStatus rc = pager_read_page(db->pager, p, &page);
        if (rc != ZEUS_OK) continue;
        if (page.header.type != PAGE_TYPE_BTREE_LEAF) continue;
        if (page.header.num_cells == 0) continue;

        uint32_t off = 0;
        bool page_modified = false;

        for (uint32_t c = 0; c < page.header.num_cells; c++) {
            uint32_t rowid;
            uint32_t data_size;
            uint32_t cell_off = off;
            memcpy(&rowid, page.data + off, sizeof(uint32_t)); off += sizeof(uint32_t);
            memcpy(&data_size, page.data + off, sizeof(uint32_t)); off += sizeof(uint32_t);

            if (data_size > 0 && data_size < MAX_ROW_SIZE) {
                Row row;
                rc = row_deserialize(page.data + off, data_size, schema, &row);
                if (rc == ZEUS_OK) {
                    bool passes = true;
                    if (stmt->has_where) {
                        passes = evaluate_where(&row, schema, stmt->where, stmt->num_where);
                    }

                    if (passes) {
                        /* SET değerlerini uygula */
                        for (uint32_t s = 0; s < stmt->num_sets; s++) {
                            for (uint32_t j = 0; j < schema->num_columns; j++) {
                                if (strcasecmp(schema->columns[j].name,
                                               stmt->set_columns[s]) == 0) {
                                    zeus_value_free(&row.values[j]);
                                    zeus_value_copy(&row.values[j], &stmt->set_values[s]);
                                    break;
                                }
                            }
                        }

                        /* Güncellenmiş satırı serialize et */
                        uint8_t new_buffer[MAX_ROW_SIZE];
                        uint32_t new_size;
                        row_serialize(&row, schema, new_buffer, &new_size);

                        /* Sayfaya geri yaz */
                        uint32_t write_off = cell_off;
                        memcpy(page.data + write_off, &rowid, sizeof(uint32_t));
                        write_off += sizeof(uint32_t);
                        memcpy(page.data + write_off, &new_size, sizeof(uint32_t));
                        write_off += sizeof(uint32_t);
                        memcpy(page.data + write_off, new_buffer, new_size);

                        page_modified = true;
                        updated++;
                    }

                    for (uint32_t v = 0; v < row.num_values; v++) {
                        zeus_value_free(&row.values[v]);
                    }
                }
                off += data_size;
            }
        }

        if (page_modified) {
            pager_write_page(db->pager, p, &page);
        }
    }

    rs->rows_affected = updated;
    rs->num_cols = 1;
    strncpy(rs->col_names[0], "Result", MAX_COLUMN_NAME - 1);

    Row row = {0};
    row.num_values = 1;
    char msg[128];
    snprintf(msg, sizeof(msg), "%u satır güncellendi", updated);
    row.values[0] = zeus_value_text(msg);
    resultset_add_row(rs, &row);
    zeus_value_free(&row.values[0]);

    return ZEUS_OK;
}

/* ============================================================
 * EXECUTE: DELETE
 * ============================================================ */

static ZeusStatus exec_delete(Database *db, const ASTDelete *stmt, ResultSet *rs) {
    TableSchema *schema = table_find(db, stmt->table_name);
    if (!schema) return ZEUS_ERROR_NOT_FOUND;

    db->total_deletes++;
    uint32_t deleted = 0;

    for (uint32_t p = 1; p < db->pager->num_pages; p++) {
        Page page;
        ZeusStatus rc = pager_read_page(db->pager, p, &page);
        if (rc != ZEUS_OK) continue;
        if (page.header.type != PAGE_TYPE_BTREE_LEAF) continue;
        if (page.header.num_cells == 0) continue;

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
                    bool passes = true;
                    if (stmt->has_where) {
                        passes = evaluate_where(&row, schema, stmt->where, stmt->num_where);
                    }

                    if (passes) {
                        /* Sayfadan sil — cell'i sıfırla */
                        page.header.num_cells--;
                        memset(page.data + off - sizeof(uint32_t) * 2, 0,
                               data_size + sizeof(uint32_t) * 2);
                        pager_write_page(db->pager, p, &page);
                        deleted++;
                        schema->row_count--;
                    }

                    for (uint32_t v = 0; v < row.num_values; v++) {
                        zeus_value_free(&row.values[v]);
                    }
                }
                off += data_size;
            }
        }
    }

    rs->rows_affected = deleted;
    rs->num_cols = 1;
    strncpy(rs->col_names[0], "Result", MAX_COLUMN_NAME - 1);

    Row row = {0};
    row.num_values = 1;
    char msg[128];
    snprintf(msg, sizeof(msg), "%u satır silindi", deleted);
    row.values[0] = zeus_value_text(msg);
    resultset_add_row(rs, &row);
    zeus_value_free(&row.values[0]);

    return ZEUS_OK;
}

/* ============================================================
 * EXECUTE: SHOW TABLES
 * ============================================================ */

static ZeusStatus exec_show_tables(Database *db, ResultSet *rs) {
    rs->num_cols = 3;
    strncpy(rs->col_names[0], "Tablo", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[1], "Sütun", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[2], "Satır", MAX_COLUMN_NAME - 1);

    for (uint32_t i = 0; i < db->num_tables; i++) {
        Row row = {0};
        row.num_values = 3;
        row.values[0] = zeus_value_text(db->tables[i].name);
        row.values[1] = zeus_value_int((int64_t)db->tables[i].num_columns);
        row.values[2] = zeus_value_int((int64_t)db->tables[i].row_count);
        resultset_add_row(rs, &row);
        zeus_value_free(&row.values[0]);
    }

    return ZEUS_OK;
}

/* ============================================================
 * EXECUTE: DESCRIBE
 * ============================================================ */

static ZeusStatus exec_describe(Database *db, const char *table_name, ResultSet *rs) {
    TableSchema *schema = table_find(db, table_name);
    if (!schema) return ZEUS_ERROR_NOT_FOUND;

    rs->num_cols = 5;
    strncpy(rs->col_names[0], "Sütun", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[1], "Tip", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[2], "Null", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[3], "Key", MAX_COLUMN_NAME - 1);
    strncpy(rs->col_names[4], "Extra", MAX_COLUMN_NAME - 1);

    for (uint32_t i = 0; i < schema->num_columns; i++) {
        const ColumnDef *col = &schema->columns[i];
        Row row = {0};
        row.num_values = 5;

        row.values[0] = zeus_value_text(col->name);

        const char *type_str = "UNKNOWN";
        switch (col->type) {
            case ZEUS_TYPE_INT:       type_str = "INT"; break;
            case ZEUS_TYPE_FLOAT:     type_str = "FLOAT"; break;
            case ZEUS_TYPE_TEXT:      type_str = "TEXT"; break;
            case ZEUS_TYPE_BOOL:      type_str = "BOOL"; break;
            case ZEUS_TYPE_BLOB:      type_str = "BLOB"; break;
            case ZEUS_TYPE_TIMESTAMP: type_str = "TIMESTAMP"; break;
            default: break;
        }
        row.values[1] = zeus_value_text(type_str);
        row.values[2] = zeus_value_text((col->flags & COL_FLAG_NOT_NULL) ? "NO" : "YES");
        row.values[3] = zeus_value_text((col->flags & COL_FLAG_PRIMARY_KEY) ? "PRI" : "");
        row.values[4] = zeus_value_text((col->flags & COL_FLAG_AUTO_INC) ? "auto_increment" : "");

        resultset_add_row(rs, &row);
        for (uint32_t v = 0; v < row.num_values; v++) {
            zeus_value_free(&row.values[v]);
        }
    }

    return ZEUS_OK;
}

/* ============================================================
 * ANA EXECUTE FONKSİYONU
 * ============================================================ */

ZeusStatus execute(Database *db, const ASTNode *ast, ResultSet *result) {
    if (!db || !ast || !result) return ZEUS_ERROR;

    uint64_t start = zeus_timestamp_us();
    ZeusStatus rc;

    switch (ast->type) {
        case AST_CREATE_TABLE:
            rc = exec_create_table(db, &ast->create_table, result);
            break;
        case AST_DROP_TABLE:
            rc = exec_drop_table(db, &ast->drop_table, result);
            break;
        case AST_INSERT:
            rc = exec_insert(db, &ast->insert_stmt, result);
            break;
        case AST_SELECT:
            rc = exec_select(db, &ast->select_stmt, result);
            break;
        case AST_UPDATE:
            rc = exec_update(db, &ast->update_stmt, result);
            break;
        case AST_DELETE:
            rc = exec_delete(db, &ast->delete_stmt, result);
            break;
        case AST_SHOW_TABLES:
            rc = exec_show_tables(db, result);
            break;
        case AST_DESCRIBE:
            rc = exec_describe(db, ast->table_name, result);
            break;
        case AST_BEGIN:
        case AST_COMMIT:
        case AST_ROLLBACK: {
            result->num_cols = 1;
            strncpy(result->col_names[0], "Result", MAX_COLUMN_NAME - 1);
            Row row = {0};
            row.num_values = 1;
            const char *msg = ast->type == AST_BEGIN ? "Transaction başlatıldı" :
                              ast->type == AST_COMMIT ? "Transaction commit edildi" :
                              "Transaction rollback edildi";
            row.values[0] = zeus_value_text(msg);
            resultset_add_row(result, &row);
            zeus_value_free(&row.values[0]);
            rc = ZEUS_OK;
            break;
        }
        default:
            zeus_log("ERROR", "Desteklenmeyen komut: %d", ast->type);
            rc = ZEUS_ERROR_SYNTAX;
            break;
    }

    result->exec_time_us = zeus_timestamp_us() - start;
    db->total_queries++;

    return rc;
}

/* ============================================================
 * EXECUTE SQL (Tokenize + Parse + Execute)
 * ============================================================ */

ZeusStatus execute_sql(Database *db, const char *sql, ResultSet *result) {
    if (!db || !sql || !result) return ZEUS_ERROR;

    /* Tokenize */
    Token *tokens = calloc(MAX_TOKENS, sizeof(Token));
    if (!tokens) return ZEUS_ERROR_NOMEM;

    uint32_t num_tokens;
    ZeusStatus rc = tokenize(sql, tokens, &num_tokens);
    if (rc != ZEUS_OK) {
        free(tokens);
        return rc;
    }

    /* Parse */
    ASTNode ast;
    rc = parse(tokens, num_tokens, &ast);
    if (rc != ZEUS_OK) {
        free(tokens);
        return rc;
    }

    /* Execute */
    rc = execute(db, &ast, result);

    free(tokens);
    return rc;
}
