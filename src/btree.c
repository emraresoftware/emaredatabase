/*
 * ZeusDB - B+Tree Implementation
 *
 * Disk-based B+Tree index yapısı.
 * - Leaf node'lar veri tutar ve birbirine bağlıdır (range scan için)
 * - Internal node'lar sadece key ve child pointer tutar
 * - Split ve merge işlemleri ile dengeli kalır
 */

#include "../include/zeusdb.h"

/* ============================================================
 * YARDIMCI FONKSİYONLAR
 * ============================================================ */

static ZeusStatus btree_node_read(BTree *tree, uint32_t page_num, BTreeNode *node) {
    Page page;
    ZeusStatus rc = pager_read_page(tree->pager, page_num, &page);
    if (rc != ZEUS_OK) return rc;

    memset(node, 0, sizeof(BTreeNode));
    node->page_num = page_num;
    node->is_leaf = (page.header.type == PAGE_TYPE_BTREE_LEAF);
    node->parent = page.header.parent_page;

    /* Sayfa verisinden node bilgilerini oku */
    uint32_t offset = 0;
    memcpy(&node->num_keys, page.data + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
    memcpy(&node->next_leaf, page.data + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
    memcpy(&node->prev_leaf, page.data + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);

    /* Key'leri oku */
    for (uint32_t i = 0; i < node->num_keys && i < BTREE_MAX_KEYS; i++) {
        memcpy(&node->keys[i], page.data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    if (node->is_leaf) {
        /* Leaf: cell'leri oku */
        for (uint32_t i = 0; i < node->num_keys && i < BTREE_MAX_KEYS; i++) {
            memcpy(&node->cells[i].key, page.data + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
            memcpy(&node->cells[i].data_offset, page.data + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
            memcpy(&node->cells[i].data_size, page.data + offset, sizeof(uint16_t)); offset += sizeof(uint16_t);
        }
    } else {
        /* Internal: child pointer'ları oku */
        for (uint32_t i = 0; i <= node->num_keys && i < BTREE_MAX_CHILDREN; i++) {
            memcpy(&node->children[i], page.data + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);
        }
    }

    return ZEUS_OK;
}

static ZeusStatus btree_node_write(BTree *tree, const BTreeNode *node) {
    Page page;
    memset(&page, 0, sizeof(Page));

    page.header.page_num = node->page_num;
    page.header.type = node->is_leaf ? PAGE_TYPE_BTREE_LEAF : PAGE_TYPE_BTREE_INTERNAL;
    page.header.num_cells = node->num_keys;
    page.header.parent_page = node->parent;

    /* Node bilgilerini sayfaya yaz */
    uint32_t offset = 0;
    memcpy(page.data + offset, &node->num_keys, sizeof(uint32_t)); offset += sizeof(uint32_t);
    memcpy(page.data + offset, &node->next_leaf, sizeof(uint32_t)); offset += sizeof(uint32_t);
    memcpy(page.data + offset, &node->prev_leaf, sizeof(uint32_t)); offset += sizeof(uint32_t);

    /* Key'leri yaz */
    for (uint32_t i = 0; i < node->num_keys; i++) {
        memcpy(page.data + offset, &node->keys[i], sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    if (node->is_leaf) {
        for (uint32_t i = 0; i < node->num_keys; i++) {
            memcpy(page.data + offset, &node->cells[i].key, sizeof(uint32_t)); offset += sizeof(uint32_t);
            memcpy(page.data + offset, &node->cells[i].data_offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
            memcpy(page.data + offset, &node->cells[i].data_size, sizeof(uint16_t)); offset += sizeof(uint16_t);
        }
    } else {
        for (uint32_t i = 0; i <= node->num_keys; i++) {
            memcpy(page.data + offset, &node->children[i], sizeof(uint32_t));
            offset += sizeof(uint32_t);
        }
    }

    page.header.free_space = PAGE_SIZE - sizeof(PageHeader) - offset;

    return pager_write_page(tree->pager, node->page_num, &page);
}

static ZeusStatus btree_node_create(BTree *tree, BTreeNode *node, bool is_leaf) {
    uint32_t page_num;
    ZeusStatus rc = pager_allocate_page(tree->pager, &page_num);
    if (rc != ZEUS_OK) return rc;

    memset(node, 0, sizeof(BTreeNode));
    node->page_num = page_num;
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    node->next_leaf = 0;
    node->prev_leaf = 0;
    node->parent = 0;

    return btree_node_write(tree, node);
}

/* Binary search ile key'in pozisyonunu bul */
static uint32_t btree_find_key_pos(const BTreeNode *node, uint32_t key) {
    if (node->num_keys == 0) return 0;

    uint32_t lo = 0, hi = node->num_keys;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (node->keys[mid] < key) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* ============================================================
 * B+TREE OLUŞTURMA / YIKMA
 * ============================================================ */

ZeusStatus btree_create(BTree **tree, Pager *pager) {
    if (!tree || !pager) return ZEUS_ERROR;

    BTree *t = calloc(1, sizeof(BTree));
    if (!t) return ZEUS_ERROR_NOMEM;

    t->pager = pager;
    t->num_entries = 0;
    t->height = 1;
    pthread_rwlock_init(&t->lock, NULL);

    /* Root node oluştur (leaf olarak başlar) */
    BTreeNode root;
    ZeusStatus rc = btree_node_create(t, &root, true);
    if (rc != ZEUS_OK) {
        free(t);
        return rc;
    }

    t->root_page = root.page_num;

    zeus_log("DEBUG", "B+Tree oluşturuldu, root: %u", t->root_page);
    *tree = t;
    return ZEUS_OK;
}

ZeusStatus btree_destroy(BTree *tree) {
    if (!tree) return ZEUS_ERROR;
    pthread_rwlock_destroy(&tree->lock);
    free(tree);
    return ZEUS_OK;
}

/* ============================================================
 * LEAF NODE SPLIT
 * ============================================================ */

static ZeusStatus btree_split_leaf(BTree *tree, BTreeNode *leaf, uint32_t key,
                                     const BTreeCell *new_cell) {
    /* Yeni sibling leaf oluştur */
    BTreeNode new_leaf;
    ZeusStatus rc = btree_node_create(tree, &new_leaf, true);
    if (rc != ZEUS_OK) return rc;

    /* Tüm key'leri geçici diziye kopyala + yeni key'i ekle */
    uint32_t total = leaf->num_keys + 1;
    uint32_t temp_keys[BTREE_MAX_KEYS + 1];
    BTreeCell temp_cells[BTREE_MAX_KEYS + 1];

    uint32_t pos = btree_find_key_pos(leaf, key);

    for (uint32_t i = 0, j = 0; j < total; j++) {
        if (j == pos) {
            temp_keys[j] = key;
            temp_cells[j] = *new_cell;
        } else {
            temp_keys[j] = leaf->keys[i];
            temp_cells[j] = leaf->cells[i];
            i++;
        }
    }

    /* Yarıdan böl */
    uint32_t split = total / 2;

    leaf->num_keys = split;
    for (uint32_t i = 0; i < split; i++) {
        leaf->keys[i] = temp_keys[i];
        leaf->cells[i] = temp_cells[i];
    }

    new_leaf.num_keys = total - split;
    for (uint32_t i = 0; i < new_leaf.num_keys; i++) {
        new_leaf.keys[i] = temp_keys[split + i];
        new_leaf.cells[i] = temp_cells[split + i];
    }

    /* Leaf bağlantılarını güncelle */
    new_leaf.next_leaf = leaf->next_leaf;
    new_leaf.prev_leaf = leaf->page_num;
    leaf->next_leaf = new_leaf.page_num;

    /* Next leaf'in prev'ini güncelle */
    if (new_leaf.next_leaf != 0) {
        BTreeNode next;
        btree_node_read(tree, new_leaf.next_leaf, &next);
        next.prev_leaf = new_leaf.page_num;
        btree_node_write(tree, &next);
    }

    /* Parent'a yeni key'i ekle */
    uint32_t promoted_key = new_leaf.keys[0];

    if (leaf->parent == 0) {
        /* Root split - yeni root oluştur */
        BTreeNode new_root;
        rc = btree_node_create(tree, &new_root, false);
        if (rc != ZEUS_OK) return rc;

        new_root.num_keys = 1;
        new_root.keys[0] = promoted_key;
        new_root.children[0] = leaf->page_num;
        new_root.children[1] = new_leaf.page_num;

        leaf->parent = new_root.page_num;
        new_leaf.parent = new_root.page_num;

        tree->root_page = new_root.page_num;
        tree->height++;

        btree_node_write(tree, &new_root);
    } else {
        /* Parent'a key ekle */
        BTreeNode parent;
        btree_node_read(tree, leaf->parent, &parent);

        new_leaf.parent = leaf->parent;

        if (parent.num_keys >= BTREE_MAX_KEYS) {
            /* Parent da dolu - recursive split gerekli */
            /* Basitleştirilmiş: parent'a eklemeyi dene */
            uint32_t ppos = btree_find_key_pos(&parent, promoted_key);

            /* Shift */
            for (uint32_t i = parent.num_keys; i > ppos; i--) {
                parent.keys[i] = parent.keys[i - 1];
                parent.children[i + 1] = parent.children[i];
            }
            parent.keys[ppos] = promoted_key;
            parent.children[ppos + 1] = new_leaf.page_num;
            parent.num_keys++;

            btree_node_write(tree, &parent);
        } else {
            uint32_t ppos = btree_find_key_pos(&parent, promoted_key);

            for (uint32_t i = parent.num_keys; i > ppos; i--) {
                parent.keys[i] = parent.keys[i - 1];
                parent.children[i + 1] = parent.children[i];
            }
            parent.keys[ppos] = promoted_key;
            parent.children[ppos + 1] = new_leaf.page_num;
            parent.num_keys++;

            btree_node_write(tree, &parent);
        }
    }

    btree_node_write(tree, leaf);
    btree_node_write(tree, &new_leaf);

    return ZEUS_OK;
}

/* ============================================================
 * INSERT
 * ============================================================ */

ZeusStatus btree_insert(BTree *tree, uint32_t key, const uint8_t *data, uint16_t data_size) {
    if (!tree) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&tree->lock);

    /* Root'tan başlayarak leaf'i bul */
    BTreeNode current;
    ZeusStatus rc = btree_node_read(tree, tree->root_page, &current);
    if (rc != ZEUS_OK) {
        pthread_rwlock_unlock(&tree->lock);
        return rc;
    }

    /* Internal node'lardan geçerek leaf'e ulaş */
    while (!current.is_leaf) {
        uint32_t pos = btree_find_key_pos(&current, key);
        uint32_t child_page;

        if (pos < current.num_keys && current.keys[pos] == key) {
            child_page = current.children[pos + 1];
        } else {
            child_page = current.children[pos];
        }

        rc = btree_node_read(tree, child_page, &current);
        if (rc != ZEUS_OK) {
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    /* Duplicate key kontrolü */
    for (uint32_t i = 0; i < current.num_keys; i++) {
        if (current.keys[i] == key) {
            pthread_rwlock_unlock(&tree->lock);
            return ZEUS_ERROR_EXISTS;
        }
    }

    /* Cell oluştur */
    BTreeCell cell;
    cell.key = key;
    cell.data_offset = 0; /* Gerçek offset sonra hesaplanacak */
    cell.data_size = data_size;

    /* Leaf'te yer var mı? */
    if (current.num_keys < BTREE_MAX_KEYS) {
        /* Doğrudan ekle */
        uint32_t pos = btree_find_key_pos(&current, key);

        /* Mevcut elemanları kaydır */
        for (uint32_t i = current.num_keys; i > pos; i--) {
            current.keys[i] = current.keys[i - 1];
            current.cells[i] = current.cells[i - 1];
        }

        current.keys[pos] = key;
        current.cells[pos] = cell;
        current.num_keys++;

        rc = btree_node_write(tree, &current);
    } else {
        /* Leaf dolu - split gerekli */
        rc = btree_split_leaf(tree, &current, key, &cell);
    }

    if (rc == ZEUS_OK) {
        tree->num_entries++;
    }

    pthread_rwlock_unlock(&tree->lock);
    return rc;
}

/* ============================================================
 * SEARCH
 * ============================================================ */

ZeusStatus btree_search(BTree *tree, uint32_t key, uint8_t *data, uint16_t *data_size) {
    if (!tree) return ZEUS_ERROR;

    pthread_rwlock_rdlock(&tree->lock);

    BTreeNode current;
    ZeusStatus rc = btree_node_read(tree, tree->root_page, &current);
    if (rc != ZEUS_OK) {
        pthread_rwlock_unlock(&tree->lock);
        return rc;
    }

    /* Leaf'e ulaş */
    while (!current.is_leaf) {
        uint32_t pos = btree_find_key_pos(&current, key);
        uint32_t child_page;

        if (pos < current.num_keys && current.keys[pos] == key) {
            child_page = current.children[pos + 1];
        } else {
            child_page = current.children[pos];
        }

        rc = btree_node_read(tree, child_page, &current);
        if (rc != ZEUS_OK) {
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    /* Leaf'te ara */
    for (uint32_t i = 0; i < current.num_keys; i++) {
        if (current.keys[i] == key) {
            if (data_size) *data_size = current.cells[i].data_size;
            /* data pointer dönüyoruz - gerçek veri ayrı page'de */
            pthread_rwlock_unlock(&tree->lock);
            return ZEUS_OK;
        }
    }

    pthread_rwlock_unlock(&tree->lock);
    return ZEUS_ERROR_NOT_FOUND;
}

/* ============================================================
 * DELETE
 * ============================================================ */

ZeusStatus btree_delete(BTree *tree, uint32_t key) {
    if (!tree) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&tree->lock);

    BTreeNode current;
    ZeusStatus rc = btree_node_read(tree, tree->root_page, &current);
    if (rc != ZEUS_OK) {
        pthread_rwlock_unlock(&tree->lock);
        return rc;
    }

    /* Leaf'e ulaş */
    while (!current.is_leaf) {
        uint32_t pos = btree_find_key_pos(&current, key);
        uint32_t child_page = current.children[pos];
        if (pos < current.num_keys && current.keys[pos] == key) {
            child_page = current.children[pos + 1];
        }
        rc = btree_node_read(tree, child_page, &current);
        if (rc != ZEUS_OK) {
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    /* Key'i bul ve sil */
    bool found = false;
    for (uint32_t i = 0; i < current.num_keys; i++) {
        if (current.keys[i] == key) {
            /* Sola kaydır */
            for (uint32_t j = i; j < current.num_keys - 1; j++) {
                current.keys[j] = current.keys[j + 1];
                current.cells[j] = current.cells[j + 1];
            }
            current.num_keys--;
            found = true;
            break;
        }
    }

    if (!found) {
        pthread_rwlock_unlock(&tree->lock);
        return ZEUS_ERROR_NOT_FOUND;
    }

    rc = btree_node_write(tree, &current);
    if (rc == ZEUS_OK) {
        tree->num_entries--;
    }

    /* Not: Basitleştirilmiş versiyon - merge/rebalance yok.
     * Production'da underflow durumunda sibling'den ödünç alma
     * veya merge gerekir. */

    pthread_rwlock_unlock(&tree->lock);
    return rc;
}

/* ============================================================
 * UPDATE
 * ============================================================ */

ZeusStatus btree_update(BTree *tree, uint32_t key, const uint8_t *data, uint16_t data_size) {
    if (!tree) return ZEUS_ERROR;

    pthread_rwlock_wrlock(&tree->lock);

    BTreeNode current;
    ZeusStatus rc = btree_node_read(tree, tree->root_page, &current);
    if (rc != ZEUS_OK) {
        pthread_rwlock_unlock(&tree->lock);
        return rc;
    }

    while (!current.is_leaf) {
        uint32_t pos = btree_find_key_pos(&current, key);
        uint32_t child_page = current.children[pos];
        if (pos < current.num_keys && current.keys[pos] == key) {
            child_page = current.children[pos + 1];
        }
        rc = btree_node_read(tree, child_page, &current);
        if (rc != ZEUS_OK) {
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    for (uint32_t i = 0; i < current.num_keys; i++) {
        if (current.keys[i] == key) {
            current.cells[i].data_size = data_size;
            rc = btree_node_write(tree, &current);
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    pthread_rwlock_unlock(&tree->lock);
    return ZEUS_ERROR_NOT_FOUND;
}

/* ============================================================
 * RANGE SCAN
 * ============================================================ */

ZeusStatus btree_scan(BTree *tree, uint32_t start_key, uint32_t end_key,
                       void (*callback)(uint32_t key, const uint8_t *data, uint16_t size, void *ctx),
                       void *ctx) {
    if (!tree || !callback) return ZEUS_ERROR;

    pthread_rwlock_rdlock(&tree->lock);

    /* Start key'in bulunduğu leaf'i bul */
    BTreeNode current;
    ZeusStatus rc = btree_node_read(tree, tree->root_page, &current);
    if (rc != ZEUS_OK) {
        pthread_rwlock_unlock(&tree->lock);
        return rc;
    }

    while (!current.is_leaf) {
        uint32_t pos = btree_find_key_pos(&current, start_key);
        rc = btree_node_read(tree, current.children[pos], &current);
        if (rc != ZEUS_OK) {
            pthread_rwlock_unlock(&tree->lock);
            return rc;
        }
    }

    /* Leaf'ler üzerinde ilerle */
    while (true) {
        for (uint32_t i = 0; i < current.num_keys; i++) {
            if (current.keys[i] >= start_key && current.keys[i] <= end_key) {
                callback(current.keys[i], NULL, current.cells[i].data_size, ctx);
            }
            if (current.keys[i] > end_key) {
                pthread_rwlock_unlock(&tree->lock);
                return ZEUS_OK;
            }
        }

        if (current.next_leaf == 0) break;

        rc = btree_node_read(tree, current.next_leaf, &current);
        if (rc != ZEUS_OK) break;
    }

    pthread_rwlock_unlock(&tree->lock);
    return ZEUS_OK;
}
