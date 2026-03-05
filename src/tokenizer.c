/*
 * ZeusDB - SQL Tokenizer
 *
 * SQL string'ini token'lara ayırır. Desteklenen:
 * - Tüm SQL anahtar kelimeleri
 * - String, integer, float literal'ları
 * - Karşılaştırma ve mantıksal operatörler
 * - Parantez, virgül, noktalı virgül
 */

#include "../include/zeusdb.h"
#include <ctype.h>

/* ============================================================
 * KEYWORD TABLOSU
 * ============================================================ */

typedef struct {
    const char *word;
    TokenType   type;
} Keyword;

static const Keyword keywords[] = {
    {"SELECT",         TOK_SELECT},
    {"FROM",           TOK_FROM},
    {"WHERE",          TOK_WHERE},
    {"INSERT",         TOK_INSERT},
    {"INTO",           TOK_INTO},
    {"VALUES",         TOK_VALUES},
    {"UPDATE",         TOK_UPDATE},
    {"SET",            TOK_SET},
    {"DELETE",         TOK_DELETE},
    {"CREATE",         TOK_CREATE},
    {"TABLE",          TOK_TABLE},
    {"DROP",           TOK_DROP},
    {"ALTER",          TOK_ALTER},
    {"ADD",            TOK_ADD},
    {"INDEX",          TOK_INDEX},
    {"PRIMARY",        TOK_PRIMARY},
    {"KEY",            TOK_KEY},
    {"NOT",            TOK_NOT},
    {"NULL",           TOK_NULL_KW},
    {"UNIQUE",         TOK_UNIQUE},
    {"DEFAULT",        TOK_DEFAULT},
    {"AUTO_INCREMENT", TOK_AUTO_INCREMENT},
    {"INT",            TOK_INT_KW},
    {"INTEGER",        TOK_INT_KW},
    {"BIGINT",         TOK_INT_KW},
    {"FLOAT",          TOK_FLOAT_KW},
    {"DOUBLE",         TOK_FLOAT_KW},
    {"REAL",           TOK_FLOAT_KW},
    {"TEXT",           TOK_TEXT_KW},
    {"VARCHAR",        TOK_VARCHAR},
    {"CHAR",           TOK_TEXT_KW},
    {"BOOL",           TOK_BOOL_KW},
    {"BOOLEAN",        TOK_BOOL_KW},
    {"BLOB",           TOK_BLOB_KW},
    {"TIMESTAMP",      TOK_TIMESTAMP_KW},
    {"DATETIME",       TOK_TIMESTAMP_KW},
    {"AND",            TOK_AND},
    {"OR",             TOK_OR},
    {"ORDER",          TOK_ORDER},
    {"BY",             TOK_BY},
    {"ASC",            TOK_ASC},
    {"DESC",           TOK_DESC},
    {"LIMIT",          TOK_LIMIT},
    {"OFFSET",         TOK_OFFSET},
    {"JOIN",           TOK_JOIN},
    {"ON",             TOK_ON},
    {"INNER",          TOK_INNER},
    {"LEFT",           TOK_LEFT},
    {"RIGHT",          TOK_RIGHT},
    {"GROUP",          TOK_GROUP},
    {"HAVING",         TOK_HAVING},
    {"AS",             TOK_AS},
    {"BEGIN",          TOK_BEGIN},
    {"COMMIT",         TOK_COMMIT},
    {"ROLLBACK",       TOK_ROLLBACK},
    {"SAVEPOINT",      TOK_SAVEPOINT},
    {"COUNT",          TOK_COUNT},
    {"SUM",            TOK_SUM},
    {"AVG",            TOK_AVG},
    {"MIN",            TOK_MIN_KW},
    {"MAX",            TOK_MAX_KW},
    {"IF",             TOK_IF},
    {"EXISTS",         TOK_EXISTS},
    {"IN",             TOK_IN},
    {"BETWEEN",        TOK_BETWEEN},
    {"LIKE",           TOK_LIKE},
    {"IS",             TOK_IS},
    {"TRUE",           TOK_TRUE_KW},
    {"FALSE",          TOK_FALSE_KW},
    {"SHOW",           TOK_SHOW},
    {"TABLES",         TOK_TABLES},
    {"DESCRIBE",       TOK_DESCRIBE},
    {"DESC",           TOK_DESCRIBE},  /* DESCRIBE kısaltması (context'e bağlı) */
    {"EXPLAIN",        TOK_EXPLAIN},
    {NULL,             TOK_ERROR}
};

/* ============================================================
 * YARDIMCI FONKSİYONLAR
 * ============================================================ */

static TokenType lookup_keyword(const char *word) {
    /* Büyük harfe çevir ve karşılaştır */
    char upper[MAX_TOKEN_LENGTH];
    size_t len = strlen(word);
    if (len >= MAX_TOKEN_LENGTH) return TOK_IDENTIFIER;

    for (size_t i = 0; i < len; i++) {
        upper[i] = toupper((unsigned char)word[i]);
    }
    upper[len] = '\0';

    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(upper, keywords[i].word) == 0) {
            return keywords[i].type;
        }
    }

    return TOK_IDENTIFIER;
}

static bool is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/* ============================================================
 * ANA TOKENIZER
 * ============================================================ */

ZeusStatus tokenize(const char *sql, Token *tokens, uint32_t *num_tokens) {
    if (!sql || !tokens || !num_tokens) return ZEUS_ERROR;

    const char *p = sql;
    uint32_t count = 0;
    uint32_t line = 1;
    uint32_t col = 1;

    while (*p && count < MAX_TOKENS - 1) {
        /* Boşlukları atla */
        while (*p && isspace((unsigned char)*p)) {
            if (*p == '\n') { line++; col = 1; }
            else { col++; }
            p++;
        }

        if (!*p) break;

        Token *tok = &tokens[count];
        tok->line = line;
        tok->col = col;

        /* Tek satır yorum: -- */
        if (*p == '-' && *(p + 1) == '-') {
            while (*p && *p != '\n') p++;
            continue;
        }

        /* Çok satırlı yorum: /* ... */ */
        if (*p == '/' && *(p + 1) == '*') {
            p += 2;
            while (*p && !(*p == '*' && *(p + 1) == '/')) {
                if (*p == '\n') { line++; col = 1; }
                p++;
            }
            if (*p) p += 2; /* */ atla */
            continue;
        }

        /* String literal: 'text' */
        if (*p == '\'') {
            p++; col++;
            uint32_t len = 0;
            while (*p && *p != '\'' && len < MAX_TOKEN_LENGTH - 1) {
                if (*p == '\\' && *(p + 1) == '\'') {
                    tok->value[len++] = '\'';
                    p += 2; col += 2;
                } else {
                    tok->value[len++] = *p;
                    p++; col++;
                }
            }
            tok->value[len] = '\0';
            tok->type = TOK_STRING_LIT;
            if (*p == '\'') { p++; col++; }
            count++;
            continue;
        }

        /* String literal: "text" */
        if (*p == '"') {
            p++; col++;
            uint32_t len = 0;
            while (*p && *p != '"' && len < MAX_TOKEN_LENGTH - 1) {
                tok->value[len++] = *p;
                p++; col++;
            }
            tok->value[len] = '\0';
            tok->type = TOK_STRING_LIT;
            if (*p == '"') { p++; col++; }
            count++;
            continue;
        }

        /* Sayılar: integer veya float */
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)*(p + 1)))) {
            uint32_t len = 0;
            bool has_dot = false;

            while (*p && (isdigit((unsigned char)*p) || (*p == '.' && !has_dot)) &&
                   len < MAX_TOKEN_LENGTH - 1) {
                if (*p == '.') has_dot = true;
                tok->value[len++] = *p;
                p++; col++;
            }
            tok->value[len] = '\0';
            tok->type = has_dot ? TOK_FLOAT_LIT : TOK_INTEGER_LIT;
            count++;
            continue;
        }

        /* Identifier veya keyword */
        if (is_ident_start(*p)) {
            uint32_t len = 0;
            while (*p && is_ident_char(*p) && len < MAX_TOKEN_LENGTH - 1) {
                tok->value[len++] = *p;
                p++; col++;
            }
            tok->value[len] = '\0';
            tok->type = lookup_keyword(tok->value);
            count++;
            continue;
        }

        /* Operatörler ve noktalama */
        switch (*p) {
            case '(':
                tok->type = TOK_LPAREN;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "(");
                break;
            case ')':
                tok->type = TOK_RPAREN;
                snprintf(tok->value, MAX_TOKEN_LENGTH, ")");
                break;
            case ',':
                tok->type = TOK_COMMA;
                snprintf(tok->value, MAX_TOKEN_LENGTH, ",");
                break;
            case ';':
                tok->type = TOK_SEMICOLON;
                snprintf(tok->value, MAX_TOKEN_LENGTH, ";");
                break;
            case '*':
                tok->type = TOK_STAR;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "*");
                break;
            case '.':
                tok->type = TOK_DOT;
                snprintf(tok->value, MAX_TOKEN_LENGTH, ".");
                break;
            case '+':
                tok->type = TOK_PLUS;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "+");
                break;
            case '-':
                tok->type = TOK_MINUS;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "-");
                break;
            case '/':
                tok->type = TOK_SLASH;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "/");
                break;
            case '=':
                tok->type = TOK_EQ;
                snprintf(tok->value, MAX_TOKEN_LENGTH, "=");
                break;
            case '<':
                if (*(p + 1) == '=') {
                    tok->type = TOK_LTE;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, "<=");
                    p++; col++;
                } else if (*(p + 1) == '>') {
                    tok->type = TOK_NEQ;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, "<>");
                    p++; col++;
                } else {
                    tok->type = TOK_LT;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, "<");
                }
                break;
            case '>':
                if (*(p + 1) == '=') {
                    tok->type = TOK_GTE;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, ">=");
                    p++; col++;
                } else {
                    tok->type = TOK_GT;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, ">");
                }
                break;
            case '!':
                if (*(p + 1) == '=') {
                    tok->type = TOK_NEQ;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, "!=");
                    p++; col++;
                } else {
                    tok->type = TOK_ERROR;
                    snprintf(tok->value, MAX_TOKEN_LENGTH, "!");
                }
                break;
            default:
                zeus_log("ERROR", "Bilinmeyen karakter: '%c' (satır %u, sütun %u)",
                         *p, line, col);
                tok->type = TOK_ERROR;
                tok->value[0] = *p;
                tok->value[1] = '\0';
                break;
        }

        p++; col++;
        count++;
    }

    /* EOF token ekle */
    tokens[count].type = TOK_EOF;
    tokens[count].value[0] = '\0';
    tokens[count].line = line;
    tokens[count].col = col;
    count++;

    *num_tokens = count;
    return ZEUS_OK;
}
