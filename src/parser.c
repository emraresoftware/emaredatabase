/*
 * ZeusDB - SQL Parser
 *
 * Token dizisini AST (Abstract Syntax Tree) yapısına dönüştürür.
 * Desteklenen SQL komutları:
 *   - SELECT ... FROM ... WHERE ... ORDER BY ... LIMIT ... OFFSET
 *   - INSERT INTO ... (cols) VALUES (vals)
 *   - UPDATE ... SET ... WHERE ...
 *   - DELETE FROM ... WHERE ...
 *   - CREATE TABLE ... (column definitions)
 *   - DROP TABLE ...
 *   - BEGIN / COMMIT / ROLLBACK
 *   - SHOW TABLES / DESCRIBE
 */

#include "../include/zeusdb.h"
#include <ctype.h>

/* ============================================================
 * PARSER CONTEXT
 * ============================================================ */

typedef struct {
    const Token *tokens;
    uint32_t     num_tokens;
    uint32_t     pos;
} ParserCtx;

/* ============================================================
 * YARDIMCI FONKSİYONLAR
 * ============================================================ */

static const Token* current(ParserCtx *ctx) {
    if (ctx->pos < ctx->num_tokens) return &ctx->tokens[ctx->pos];
    return NULL;
}

static const Token* peek(ParserCtx *ctx) {
    if (ctx->pos + 1 < ctx->num_tokens) return &ctx->tokens[ctx->pos + 1];
    return NULL;
}

static bool at_end(ParserCtx *ctx) {
    return ctx->pos >= ctx->num_tokens || ctx->tokens[ctx->pos].type == TOK_EOF;
}

static const Token* advance(ParserCtx *ctx) {
    if (ctx->pos < ctx->num_tokens) {
        return &ctx->tokens[ctx->pos++];
    }
    return NULL;
}

static bool check(ParserCtx *ctx, TokenType type) {
    if (at_end(ctx)) return false;
    return ctx->tokens[ctx->pos].type == type;
}

static bool match(ParserCtx *ctx, TokenType type) {
    if (check(ctx, type)) {
        advance(ctx);
        return true;
    }
    return false;
}

static ZeusStatus expect(ParserCtx *ctx, TokenType type, const char *msg) {
    if (check(ctx, type)) {
        advance(ctx);
        return ZEUS_OK;
    }
    zeus_log("ERROR", "SQL sözdizimi hatası: %s (satır %u, sütun %u, bulunan: '%s')",
             msg, ctx->tokens[ctx->pos].line, ctx->tokens[ctx->pos].col,
             ctx->tokens[ctx->pos].value);
    return ZEUS_ERROR_SYNTAX;
}

/* ============================================================
 * VALUE PARSING
 * ============================================================ */

static ZeusStatus parse_value(ParserCtx *ctx, ZeusValue *val) {
    const Token *tok = current(ctx);
    if (!tok) return ZEUS_ERROR_SYNTAX;

    if (tok->type == TOK_INTEGER_LIT) {
        val->type = ZEUS_TYPE_INT;
        val->int_val = atoll(tok->value);
        val->is_null = false;
        advance(ctx);
        return ZEUS_OK;
    }

    if (tok->type == TOK_FLOAT_LIT) {
        val->type = ZEUS_TYPE_FLOAT;
        val->float_val = atof(tok->value);
        val->is_null = false;
        advance(ctx);
        return ZEUS_OK;
    }

    if (tok->type == TOK_STRING_LIT) {
        val->type = ZEUS_TYPE_TEXT;
        val->str_val.data = strdup(tok->value);
        val->str_val.len = (uint32_t)strlen(tok->value);
        val->is_null = false;
        advance(ctx);
        return ZEUS_OK;
    }

    if (tok->type == TOK_NULL_KW) {
        val->type = ZEUS_TYPE_NULL;
        val->is_null = true;
        advance(ctx);
        return ZEUS_OK;
    }

    if (tok->type == TOK_TRUE_KW) {
        val->type = ZEUS_TYPE_BOOL;
        val->bool_val = true;
        val->is_null = false;
        advance(ctx);
        return ZEUS_OK;
    }

    if (tok->type == TOK_FALSE_KW) {
        val->type = ZEUS_TYPE_BOOL;
        val->bool_val = false;
        val->is_null = false;
        advance(ctx);
        return ZEUS_OK;
    }

    /* Negatif sayı: -123 */
    if (tok->type == TOK_MINUS) {
        advance(ctx);
        tok = current(ctx);
        if (tok && tok->type == TOK_INTEGER_LIT) {
            val->type = ZEUS_TYPE_INT;
            val->int_val = -atoll(tok->value);
            val->is_null = false;
            advance(ctx);
            return ZEUS_OK;
        }
        if (tok && tok->type == TOK_FLOAT_LIT) {
            val->type = ZEUS_TYPE_FLOAT;
            val->float_val = -atof(tok->value);
            val->is_null = false;
            advance(ctx);
            return ZEUS_OK;
        }
    }

    return ZEUS_ERROR_SYNTAX;
}

/* ============================================================
 * WHERE CLAUSE PARSING
 * ============================================================ */

static CompareOp token_to_cmp(TokenType type) {
    switch (type) {
        case TOK_EQ:   return CMP_EQ;
        case TOK_NEQ:  return CMP_NEQ;
        case TOK_LT:   return CMP_LT;
        case TOK_GT:   return CMP_GT;
        case TOK_LTE:  return CMP_LTE;
        case TOK_GTE:  return CMP_GTE;
        case TOK_LIKE: return CMP_LIKE;
        default:       return CMP_EQ;
    }
}

static ZeusStatus parse_where(ParserCtx *ctx, WhereClause *clauses, uint32_t *num_clauses) {
    *num_clauses = 0;

    while (!at_end(ctx) && *num_clauses < MAX_WHERE_CONDITIONS) {
        WhereClause *wc = &clauses[*num_clauses];
        memset(wc, 0, sizeof(WhereClause));

        /* column name */
        const Token *tok = current(ctx);
        if (!tok || tok->type != TOK_IDENTIFIER) {
            return ZEUS_ERROR_SYNTAX;
        }
        strncpy(wc->column, tok->value, MAX_COLUMN_NAME - 1);
        advance(ctx);

        /* IS NULL / IS NOT NULL */
        if (check(ctx, TOK_IS)) {
            advance(ctx);
            if (check(ctx, TOK_NOT)) {
                advance(ctx);
                expect(ctx, TOK_NULL_KW, "'NULL' bekleniyor");
                wc->op = CMP_IS_NOT_NULL;
            } else {
                expect(ctx, TOK_NULL_KW, "'NULL' bekleniyor");
                wc->op = CMP_IS_NULL;
            }
        }
        /* BETWEEN x AND y */
        else if (check(ctx, TOK_BETWEEN)) {
            advance(ctx);
            wc->op = CMP_BETWEEN;
            ZeusStatus rc = parse_value(ctx, &wc->value);
            if (rc != ZEUS_OK) return rc;
            expect(ctx, TOK_AND, "'AND' bekleniyor (BETWEEN)");
            rc = parse_value(ctx, &wc->value2);
            if (rc != ZEUS_OK) return rc;
        }
        /* LIKE 'pattern' */
        else if (check(ctx, TOK_LIKE)) {
            advance(ctx);
            wc->op = CMP_LIKE;
            ZeusStatus rc = parse_value(ctx, &wc->value);
            if (rc != ZEUS_OK) return rc;
        }
        /* Karşılaştırma operatörleri: =, !=, <, >, <=, >= */
        else {
            tok = current(ctx);
            if (!tok) return ZEUS_ERROR_SYNTAX;

            if (tok->type >= TOK_EQ && tok->type <= TOK_GTE) {
                wc->op = token_to_cmp(tok->type);
                advance(ctx);
                ZeusStatus rc = parse_value(ctx, &wc->value);
                if (rc != ZEUS_OK) return rc;
            } else {
                zeus_log("ERROR", "Karşılaştırma operatörü bekleniyor, bulunan: '%s'", tok->value);
                return ZEUS_ERROR_SYNTAX;
            }
        }

        (*num_clauses)++;

        /* AND / OR bağlacı */
        if (check(ctx, TOK_AND)) {
            wc->logic = LOGIC_AND;
            advance(ctx);
        } else if (check(ctx, TOK_OR)) {
            wc->logic = LOGIC_OR;
            advance(ctx);
        } else {
            break;
        }
    }

    return ZEUS_OK;
}

/* ============================================================
 * PARSE: CREATE TABLE
 * ============================================================ */

static ZeusStatus parse_create_table(ParserCtx *ctx, ASTNode *ast) {
    ast->type = AST_CREATE_TABLE;
    ASTCreateTable *ct = &ast->create_table;
    memset(ct, 0, sizeof(ASTCreateTable));

    /* IF NOT EXISTS */
    if (check(ctx, TOK_IF)) {
        advance(ctx);
        expect(ctx, TOK_NOT, "'NOT' bekleniyor");
        expect(ctx, TOK_EXISTS, "'EXISTS' bekleniyor");
        ct->if_not_exists = true;
    }

    /* Tablo adı */
    const Token *tok = current(ctx);
    if (!tok || tok->type != TOK_IDENTIFIER) {
        return ZEUS_ERROR_SYNTAX;
    }
    strncpy(ct->table_name, tok->value, MAX_TABLE_NAME - 1);
    advance(ctx);

    /* ( */
    ZeusStatus rc = expect(ctx, TOK_LPAREN, "'(' bekleniyor");
    if (rc != ZEUS_OK) return rc;

    /* Sütun tanımları */
    while (!at_end(ctx) && !check(ctx, TOK_RPAREN)) {
        if (ct->num_columns >= MAX_COLUMNS) {
            zeus_log("ERROR", "Maksimum sütun sayısı aşıldı (%u)", MAX_COLUMNS);
            return ZEUS_ERROR_OVERFLOW;
        }

        /* PRIMARY KEY constraint (tablo seviyesi) */
        if (check(ctx, TOK_PRIMARY)) {
            advance(ctx);
            expect(ctx, TOK_KEY, "'KEY' bekleniyor");
            expect(ctx, TOK_LPAREN, "'(' bekleniyor");
            tok = current(ctx);
            if (tok && tok->type == TOK_IDENTIFIER) {
                /* PK sütununu bul ve işaretle */
                for (uint32_t i = 0; i < ct->num_columns; i++) {
                    if (strcasecmp(ct->columns[i].name, tok->value) == 0) {
                        ct->columns[i].flags |= COL_FLAG_PRIMARY_KEY | COL_FLAG_NOT_NULL;
                        break;
                    }
                }
                advance(ctx);
            }
            expect(ctx, TOK_RPAREN, "')' bekleniyor");

            if (check(ctx, TOK_COMMA)) advance(ctx);
            continue;
        }

        ColumnDef *col = &ct->columns[ct->num_columns];
        memset(col, 0, sizeof(ColumnDef));
        col->col_index = ct->num_columns;

        /* Sütun adı */
        tok = current(ctx);
        if (!tok || tok->type != TOK_IDENTIFIER) {
            zeus_log("ERROR", "Sütun adı bekleniyor");
            return ZEUS_ERROR_SYNTAX;
        }
        strncpy(col->name, tok->value, MAX_COLUMN_NAME - 1);
        advance(ctx);

        /* Veri tipi */
        tok = current(ctx);
        if (!tok) return ZEUS_ERROR_SYNTAX;

        switch (tok->type) {
            case TOK_INT_KW:
                col->type = ZEUS_TYPE_INT;
                advance(ctx);
                break;
            case TOK_FLOAT_KW:
                col->type = ZEUS_TYPE_FLOAT;
                advance(ctx);
                break;
            case TOK_TEXT_KW:
                col->type = ZEUS_TYPE_TEXT;
                col->max_len = MAX_VARCHAR_LEN;
                advance(ctx);
                break;
            case TOK_VARCHAR:
                col->type = ZEUS_TYPE_TEXT;
                advance(ctx);
                if (check(ctx, TOK_LPAREN)) {
                    advance(ctx);
                    tok = current(ctx);
                    if (tok && tok->type == TOK_INTEGER_LIT) {
                        col->max_len = (uint32_t)atoi(tok->value);
                        advance(ctx);
                    }
                    expect(ctx, TOK_RPAREN, "')' bekleniyor");
                } else {
                    col->max_len = 255;
                }
                break;
            case TOK_BOOL_KW:
                col->type = ZEUS_TYPE_BOOL;
                advance(ctx);
                break;
            case TOK_BLOB_KW:
                col->type = ZEUS_TYPE_BLOB;
                advance(ctx);
                break;
            case TOK_TIMESTAMP_KW:
                col->type = ZEUS_TYPE_TIMESTAMP;
                advance(ctx);
                break;
            default:
                zeus_log("ERROR", "Bilinmeyen veri tipi: '%s'", tok->value);
                return ZEUS_ERROR_SYNTAX;
        }

        /* Sütun constraint'leri */
        while (!at_end(ctx) && !check(ctx, TOK_COMMA) && !check(ctx, TOK_RPAREN)) {
            if (check(ctx, TOK_PRIMARY)) {
                advance(ctx);
                expect(ctx, TOK_KEY, "'KEY' bekleniyor");
                col->flags |= COL_FLAG_PRIMARY_KEY | COL_FLAG_NOT_NULL;
            } else if (check(ctx, TOK_NOT)) {
                advance(ctx);
                expect(ctx, TOK_NULL_KW, "'NULL' bekleniyor");
                col->flags |= COL_FLAG_NOT_NULL;
            } else if (check(ctx, TOK_UNIQUE)) {
                advance(ctx);
                col->flags |= COL_FLAG_UNIQUE;
            } else if (check(ctx, TOK_AUTO_INCREMENT)) {
                advance(ctx);
                col->flags |= COL_FLAG_AUTO_INC;
            } else if (check(ctx, TOK_DEFAULT)) {
                advance(ctx);
                col->flags |= COL_FLAG_DEFAULT;
                parse_value(ctx, &col->default_val);
            } else {
                break;
            }
        }

        ct->num_columns++;

        if (check(ctx, TOK_COMMA)) {
            advance(ctx);
        }
    }

    /* ) */
    expect(ctx, TOK_RPAREN, "')' bekleniyor");

    return ZEUS_OK;
}

/* ============================================================
 * PARSE: SELECT
 * ============================================================ */

static ZeusStatus parse_select(ParserCtx *ctx, ASTNode *ast) {
    ast->type = AST_SELECT;
    ASTSelect *sel = &ast->select_stmt;
    memset(sel, 0, sizeof(ASTSelect));
    sel->limit = -1;
    sel->offset = 0;

    /* Sütunlar */
    while (!at_end(ctx) && !check(ctx, TOK_FROM)) {
        if (sel->num_columns >= MAX_COLUMNS) break;

        SelectColumn *sc = &sel->columns[sel->num_columns];
        memset(sc, 0, sizeof(SelectColumn));

        /* * (tüm sütunlar) */
        if (check(ctx, TOK_STAR)) {
            sc->is_star = true;
            strncpy(sc->name, "*", sizeof(sc->name) - 1);
            advance(ctx);
            sel->num_columns++;
            break;
        }

        /* Aggregate fonksiyonları: COUNT(), SUM(), AVG(), MIN(), MAX() */
        if (check(ctx, TOK_COUNT) || check(ctx, TOK_SUM) ||
            check(ctx, TOK_AVG) || check(ctx, TOK_MIN_KW) || check(ctx, TOK_MAX_KW)) {
            sc->is_aggregate = true;
            sc->aggregate_func = current(ctx)->type;
            advance(ctx);
            expect(ctx, TOK_LPAREN, "'(' bekleniyor");

            if (check(ctx, TOK_STAR)) {
                strncpy(sc->name, "*", sizeof(sc->name) - 1);
                advance(ctx);
            } else {
                const Token *tok = current(ctx);
                if (tok && tok->type == TOK_IDENTIFIER) {
                    strncpy(sc->name, tok->value, MAX_COLUMN_NAME - 1);
                    advance(ctx);
                }
            }
            expect(ctx, TOK_RPAREN, "')' bekleniyor");
        } else {
            /* Normal sütun */
            const Token *tok = current(ctx);
            if (tok && tok->type == TOK_IDENTIFIER) {
                strncpy(sc->name, tok->value, MAX_COLUMN_NAME - 1);
                advance(ctx);

                /* table.column */
                if (check(ctx, TOK_DOT)) {
                    advance(ctx);
                    strncpy(sc->table, sc->name, MAX_TABLE_NAME - 1);
                    tok = current(ctx);
                    if (tok && tok->type == TOK_IDENTIFIER) {
                        strncpy(sc->name, tok->value, MAX_COLUMN_NAME - 1);
                        advance(ctx);
                    }
                }
            }
        }

        /* AS alias */
        if (check(ctx, TOK_AS)) {
            advance(ctx);
            const Token *tok = current(ctx);
            if (tok && tok->type == TOK_IDENTIFIER) {
                strncpy(sc->alias, tok->value, MAX_COLUMN_NAME - 1);
                advance(ctx);
            }
        }

        sel->num_columns++;

        if (check(ctx, TOK_COMMA)) {
            advance(ctx);
        }
    }

    /* FROM */
    if (!match(ctx, TOK_FROM)) {
        zeus_log("ERROR", "'FROM' bekleniyor");
        return ZEUS_ERROR_SYNTAX;
    }

    const Token *tok = current(ctx);
    if (tok && tok->type == TOK_IDENTIFIER) {
        strncpy(sel->table_name, tok->value, MAX_TABLE_NAME - 1);
        advance(ctx);
    } else {
        return ZEUS_ERROR_SYNTAX;
    }

    /* WHERE */
    if (check(ctx, TOK_WHERE)) {
        advance(ctx);
        sel->has_where = true;
        ZeusStatus rc = parse_where(ctx, sel->where, &sel->num_where);
        if (rc != ZEUS_OK) return rc;
    }

    /* ORDER BY */
    if (check(ctx, TOK_ORDER)) {
        advance(ctx);
        expect(ctx, TOK_BY, "'BY' bekleniyor");
        sel->has_order_by = true;

        while (!at_end(ctx) && sel->num_order_by < MAX_ORDER_BY) {
            tok = current(ctx);
            if (tok && tok->type == TOK_IDENTIFIER) {
                strncpy(sel->order_by[sel->num_order_by].column, tok->value, MAX_COLUMN_NAME - 1);
                advance(ctx);

                if (check(ctx, TOK_DESC) || check(ctx, TOK_DESCRIBE)) {
                    sel->order_by[sel->num_order_by].descending = true;
                    advance(ctx);
                } else if (check(ctx, TOK_ASC)) {
                    advance(ctx);
                }

                sel->num_order_by++;

                if (check(ctx, TOK_COMMA)) {
                    advance(ctx);
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    /* LIMIT */
    if (check(ctx, TOK_LIMIT)) {
        advance(ctx);
        sel->has_limit = true;
        tok = current(ctx);
        if (tok && tok->type == TOK_INTEGER_LIT) {
            sel->limit = atoll(tok->value);
            advance(ctx);
        }
    }

    /* OFFSET */
    if (check(ctx, TOK_OFFSET)) {
        advance(ctx);
        sel->has_offset = true;
        tok = current(ctx);
        if (tok && tok->type == TOK_INTEGER_LIT) {
            sel->offset = atoll(tok->value);
            advance(ctx);
        }
    }

    return ZEUS_OK;
}

/* ============================================================
 * PARSE: INSERT
 * ============================================================ */

static ZeusStatus parse_insert(ParserCtx *ctx, ASTNode *ast) {
    ast->type = AST_INSERT;
    ASTInsert *ins = &ast->insert_stmt;
    memset(ins, 0, sizeof(ASTInsert));

    /* INTO */
    expect(ctx, TOK_INTO, "'INTO' bekleniyor");

    /* Tablo adı */
    const Token *tok = current(ctx);
    if (!tok || tok->type != TOK_IDENTIFIER) return ZEUS_ERROR_SYNTAX;
    strncpy(ins->table_name, tok->value, MAX_TABLE_NAME - 1);
    advance(ctx);

    /* Opsiyonel sütun listesi: (col1, col2, ...) */
    if (check(ctx, TOK_LPAREN) && !check(ctx, TOK_VALUES)) {
        ins->has_columns = true;

        /* VALUES olup olmadığını kontrol et - lookahead */
        const Token *next = peek(ctx);
        if (next && (next->type == TOK_IDENTIFIER)) {
            advance(ctx); /* ( */
            while (!at_end(ctx) && !check(ctx, TOK_RPAREN)) {
                tok = current(ctx);
                if (tok && tok->type == TOK_IDENTIFIER) {
                    strncpy(ins->columns[ins->num_columns], tok->value, MAX_COLUMN_NAME - 1);
                    ins->num_columns++;
                    advance(ctx);
                }
                if (check(ctx, TOK_COMMA)) advance(ctx);
            }
            expect(ctx, TOK_RPAREN, "')' bekleniyor");
        } else {
            ins->has_columns = false;
        }
    }

    /* VALUES */
    expect(ctx, TOK_VALUES, "'VALUES' bekleniyor");
    expect(ctx, TOK_LPAREN, "'(' bekleniyor");

    while (!at_end(ctx) && !check(ctx, TOK_RPAREN)) {
        if (ins->num_values >= MAX_COLUMNS) break;
        ZeusStatus rc = parse_value(ctx, &ins->values[ins->num_values]);
        if (rc != ZEUS_OK) return rc;
        ins->num_values++;
        if (check(ctx, TOK_COMMA)) advance(ctx);
    }

    expect(ctx, TOK_RPAREN, "')' bekleniyor");

    return ZEUS_OK;
}

/* ============================================================
 * PARSE: UPDATE
 * ============================================================ */

static ZeusStatus parse_update(ParserCtx *ctx, ASTNode *ast) {
    ast->type = AST_UPDATE;
    ASTUpdate *upd = &ast->update_stmt;
    memset(upd, 0, sizeof(ASTUpdate));

    /* Tablo adı */
    const Token *tok = current(ctx);
    if (!tok || tok->type != TOK_IDENTIFIER) return ZEUS_ERROR_SYNTAX;
    strncpy(upd->table_name, tok->value, MAX_TABLE_NAME - 1);
    advance(ctx);

    /* SET */
    expect(ctx, TOK_SET, "'SET' bekleniyor");

    /* col = val, col = val, ... */
    while (!at_end(ctx) && !check(ctx, TOK_WHERE) && !check(ctx, TOK_SEMICOLON)) {
        if (upd->num_sets >= MAX_COLUMNS) break;

        tok = current(ctx);
        if (!tok || tok->type != TOK_IDENTIFIER) break;
        strncpy(upd->set_columns[upd->num_sets], tok->value, MAX_COLUMN_NAME - 1);
        advance(ctx);

        expect(ctx, TOK_EQ, "'=' bekleniyor");

        ZeusStatus rc = parse_value(ctx, &upd->set_values[upd->num_sets]);
        if (rc != ZEUS_OK) return rc;

        upd->num_sets++;

        if (check(ctx, TOK_COMMA)) advance(ctx);
    }

    /* WHERE */
    if (check(ctx, TOK_WHERE)) {
        advance(ctx);
        upd->has_where = true;
        return parse_where(ctx, upd->where, &upd->num_where);
    }

    return ZEUS_OK;
}

/* ============================================================
 * PARSE: DELETE
 * ============================================================ */

static ZeusStatus parse_delete(ParserCtx *ctx, ASTNode *ast) {
    ast->type = AST_DELETE;
    ASTDelete *del = &ast->delete_stmt;
    memset(del, 0, sizeof(ASTDelete));

    /* FROM */
    expect(ctx, TOK_FROM, "'FROM' bekleniyor");

    /* Tablo adı */
    const Token *tok = current(ctx);
    if (!tok || tok->type != TOK_IDENTIFIER) return ZEUS_ERROR_SYNTAX;
    strncpy(del->table_name, tok->value, MAX_TABLE_NAME - 1);
    advance(ctx);

    /* WHERE */
    if (check(ctx, TOK_WHERE)) {
        advance(ctx);
        del->has_where = true;
        return parse_where(ctx, del->where, &del->num_where);
    }

    return ZEUS_OK;
}

/* ============================================================
 * ANA PARSE FONKSİYONU
 * ============================================================ */

ZeusStatus parse(const Token *tokens, uint32_t num_tokens, ASTNode *ast) {
    if (!tokens || !ast || num_tokens == 0) return ZEUS_ERROR;

    ParserCtx ctx = {
        .tokens = tokens,
        .num_tokens = num_tokens,
        .pos = 0
    };

    memset(ast, 0, sizeof(ASTNode));

    const Token *tok = current(&ctx);
    if (!tok) return ZEUS_ERROR_SYNTAX;

    ZeusStatus rc;

    switch (tok->type) {
        case TOK_SELECT:
            advance(&ctx);
            rc = parse_select(&ctx, ast);
            break;

        case TOK_INSERT:
            advance(&ctx);
            rc = parse_insert(&ctx, ast);
            break;

        case TOK_UPDATE:
            advance(&ctx);
            rc = parse_update(&ctx, ast);
            break;

        case TOK_DELETE:
            advance(&ctx);
            rc = parse_delete(&ctx, ast);
            break;

        case TOK_CREATE:
            advance(&ctx);
            if (check(&ctx, TOK_TABLE)) {
                advance(&ctx);
                rc = parse_create_table(&ctx, ast);
            } else {
                zeus_log("ERROR", "CREATE sonrası 'TABLE' bekleniyor");
                rc = ZEUS_ERROR_SYNTAX;
            }
            break;

        case TOK_DROP:
            advance(&ctx);
            if (check(&ctx, TOK_TABLE)) {
                advance(&ctx);
                ast->type = AST_DROP_TABLE;
                memset(&ast->drop_table, 0, sizeof(ASTDropTable));

                if (check(&ctx, TOK_IF)) {
                    advance(&ctx);
                    expect(&ctx, TOK_EXISTS, "'EXISTS' bekleniyor");
                    ast->drop_table.if_exists = true;
                }

                tok = current(&ctx);
                if (tok && tok->type == TOK_IDENTIFIER) {
                    strncpy(ast->drop_table.table_name, tok->value, MAX_TABLE_NAME - 1);
                    advance(&ctx);
                }
                rc = ZEUS_OK;
            } else {
                rc = ZEUS_ERROR_SYNTAX;
            }
            break;

        case TOK_BEGIN:
            advance(&ctx);
            ast->type = AST_BEGIN;
            rc = ZEUS_OK;
            break;

        case TOK_COMMIT:
            advance(&ctx);
            ast->type = AST_COMMIT;
            rc = ZEUS_OK;
            break;

        case TOK_ROLLBACK:
            advance(&ctx);
            ast->type = AST_ROLLBACK;
            rc = ZEUS_OK;
            break;

        case TOK_SHOW:
            advance(&ctx);
            if (check(&ctx, TOK_TABLES)) {
                advance(&ctx);
                ast->type = AST_SHOW_TABLES;
                rc = ZEUS_OK;
            } else {
                rc = ZEUS_ERROR_SYNTAX;
            }
            break;

        case TOK_DESCRIBE:
            advance(&ctx);
            ast->type = AST_DESCRIBE;
            tok = current(&ctx);
            if (tok && tok->type == TOK_IDENTIFIER) {
                strncpy(ast->table_name, tok->value, MAX_TABLE_NAME - 1);
                advance(&ctx);
                rc = ZEUS_OK;
            } else {
                rc = ZEUS_ERROR_SYNTAX;
            }
            break;

        case TOK_EXPLAIN:
            advance(&ctx);
            if (check(&ctx, TOK_SELECT)) {
                advance(&ctx);
                rc = parse_select(&ctx, ast);
                ast->select_stmt.explain = true;
            } else {
                ast->type = AST_EXPLAIN;
                rc = ZEUS_OK;
            }
            break;

        default:
            zeus_log("ERROR", "Beklenmeyen token: '%s' (tip: %d)", tok->value, tok->type);
            rc = ZEUS_ERROR_SYNTAX;
            break;
    }

    return rc;
}
