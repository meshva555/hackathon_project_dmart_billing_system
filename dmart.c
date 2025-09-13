#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
#endif

#define PRODUCTS_FILE "products.txt"
#define RECEIPTS_FILE "receipts.txt"
#define SALES_ITEMS_FILE "sales_items.txt"
#define BILLS_DIR "bills"
#define MAX_PRODUCTS 2000
#define MAX_CART 200
#define MAX_LINE 512

/* ---------- Data Structures ---------- */
typedef struct {
    int code;
    char name[128];
    double price;
    int stock;
    double discount; // percentage
    char category[64];
    char subcategory[64];
} Product;

typedef struct {
    int code;
    char name[128];
    int qty;
    double priceAfterDisc;
    double total;
} CartItem;

/* ---------- Customer Management Data Structures ---------- */
typedef struct {
    int id;
    char name[50];
    char phone[15];
    char email[50];
    char address[100];
} Customer;

/* ---------- Prototypes ---------- */
/* general */
void clear_console();
void trimnewline(char *s);
void strtolower(char *s);

/* product load/save */
int load_products(Product products[], int maxProducts);
int save_products(Product products[], int count);
Product* find_product_by_code(Product products[], int count, int code);

/* admin */
void admin_menu();
void admin_add_product();
void admin_view_products();
void admin_view_by_category();
void admin_update_product();
void admin_delete_product();
void admin_low_stock_alerts();

/* billing */
void billing_menu();
void billing_add_item_flow(Product products[], int prodCount);
void billing_print_steady(CartItem cart[], int cartCount, const char *customerName);
void billing_finalize_and_save(CartItem cart[], int cartCount, const char *customerName);

/* reports */
void report_menu();
void report_total_income();
void report_daily_income();
void report_monthly_income();
void report_product_wise();
void report_top_selling();

/* receipts & helper */
int next_receipt_id();
void append_receipt_items(CartItem cart[], int cartCount, const char *customerName, const char *iso);
void append_sales_items(CartItem cart[], int cartCount, const char *iso);
void ensure_bills_dir();

/* simple console helpers */
void pause_console();
void view_customers();

/* ---------- Implementation ---------- */

void clear_console() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void trimnewline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) { s[n-1] = '\0'; n--; }
}

void strtolower(char *s) {
    for (; *s; ++s) if (*s >= 'A' && *s <= 'Z') *s = *s - 'A' + 'a';
}

/* ---------- Products load/save ---------- */
int load_products(Product products[], int maxProducts) {
    FILE *fp = fopen(PRODUCTS_FILE, "r");
    if (!fp) return 0;
    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < maxProducts) {
        trimnewline(line);
        if (strlen(line) == 0) continue;
        Product p;
        memset(&p, 0, sizeof(p));
        p.price = 0.0;
        p.stock = 0;
        p.discount = 0.0;
        int fields = sscanf(line, "%d,%127[^,],%lf,%d,%lf,%63[^,],%63[^\n]",
               &p.code, p.name, &p.price, &p.stock, &p.discount, p.category, p.subcategory);
        if (fields >= 5) {
            if (fields < 7) {
                if (strlen(p.category) == 0) strcpy(p.category, "Uncategorized");
                if (strlen(p.subcategory) == 0) strcpy(p.subcategory, "General");
            }
            products[count++] = p;
        } else {
            char *tok;
            char tmp[MAX_LINE];
            strncpy(tmp, line, sizeof(tmp)-1); tmp[sizeof(tmp)-1] = '\0';
            tok = strtok(tmp, ",");
            if (!tok) continue;
            p.code = atoi(tok);
            tok = strtok(NULL, ","); if (!tok) continue; strncpy(p.name, tok, sizeof(p.name));
            tok = strtok(NULL, ","); if (tok) p.price = atof(tok);
            tok = strtok(NULL, ","); if (tok) p.stock = atoi(tok);
            tok = strtok(NULL, ","); if (tok) p.discount = atof(tok);
            strncpy(p.category, "Uncategorized", sizeof(p.category));
            strncpy(p.subcategory, "General", sizeof(p.subcategory));
            products[count++] = p;
        }
    }
    fclose(fp);
    return count;
}

int save_products(Product products[], int count) {
    FILE *fp = fopen(PRODUCTS_FILE, "w");
    if (!fp) return 0;
    for (int i = 0; i < count; ++i) {
        fprintf(fp, "%d,%s,%.2f,%d,%.2f,%s,%s\n",
                products[i].code, products[i].name, products[i].price, products[i].stock,
                products[i].discount, products[i].category, products[i].subcategory);
    }
    fclose(fp);
    return 1;
}

Product* find_product_by_code(Product products[], int count, int code) {
    for (int i = 0; i < count; ++i) if (products[i].code == code) return &products[i];
    return NULL;
}

/* ---------- Admin functions ---------- */

void admin_menu() {
    while (1) {
        printf("\n==== STORE PERSON MENU ====\n");
        printf("1. Add Product\n");
        printf("2. View All Products\n");
        printf("3. View Products by Category/Subcategory\n");
        printf("4. Update Product\n");
        printf("5. Delete Product\n");
        printf("6. Low-stock Alerts (stock < 5)\n");
        printf("7. Reports Menu\n");
        printf("0. Back to Role Selection\n");
        printf("Enter choice: ");
        int ch;
        if (scanf("%d", &ch) != 1) { while (getchar() != '\n'); ch = -1; }
        while (getchar() != '\n');
        if (ch == 0) return;
        switch (ch) {
            case 1: admin_add_product(); break;
            case 2: admin_view_products(); break;
            case 3: admin_view_by_category(); break;
            case 4: admin_update_product(); break;
            case 5: admin_delete_product(); break;
            case 6: admin_low_stock_alerts(); break;
            case 7: report_menu(); break;
            default: printf("Invalid choice.\n");
        }
    }
}

void admin_add_product() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    Product p;
    printf("Enter product code (int): ");
    if (scanf("%d", &p.code) != 1) { printf("Invalid.\n"); while(getchar()!='\n'); return; }
    while(getchar()!='\n');
    if (find_product_by_code(products, n, p.code)) { printf("Product code exists.\n"); return; }
    printf("Enter product name: ");
    fgets(p.name, sizeof(p.name), stdin); trimnewline(p.name);
    printf("Enter price (e.g. 99.99): "); scanf("%lf", &p.price); while(getchar()!='\n');
    printf("Enter stock (int): "); scanf("%d", &p.stock); while(getchar()!='\n');
    printf("Enter discount percentage (e.g. 5.0): "); scanf("%lf", &p.discount); while(getchar()!='\n');
    printf("Enter category: "); fgets(p.category, sizeof(p.category), stdin); trimnewline(p.category);
    printf("Enter subcategory: "); fgets(p.subcategory, sizeof(p.subcategory), stdin); trimnewline(p.subcategory);
    products[n++] = p;
    if (!save_products(products, n)) printf("Failed to save products.\n");
    else printf("Product added.\n");
}

void admin_view_products() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    if (n == 0) { printf("No products found.\n"); return; }
    printf("\nCode | Name | Price | Stock | Disc | Category > Sub\n");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        printf("%4d | % -25s | %7.2f | %5d | %4.1f%% | %s > %s\n",
               products[i].code, products[i].name, products[i].price, products[i].stock,
               products[i].discount, products[i].category, products[i].subcategory);
    }
    printf("Total products: %d\n", n);
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
    printf("| Code | Name                      | Price   | Stock | Disc%%  | Category          | Subcategory        |\n");
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
    for (int i = 0; i < n; ++i) {
        printf("| %-4d | %-25s | %7.2f | %5d | %5.1f | %-17s | %-17s |\n",
               products[i].code, products[i].name, products[i].price, products[i].stock,
               products[i].discount, products[i].category, products[i].subcategory);
    }
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
}

void admin_view_by_category() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    if (n == 0) { printf("No products.\n"); return; }
    printf("Enter category (or 'all'): ");
    char cat[64]; fgets(cat, sizeof(cat), stdin); trimnewline(cat);
    printf("Enter subcategory (or 'all'): ");
    char sub[64]; fgets(sub, sizeof(sub), stdin); trimnewline(sub);
    int cnt = 0;
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
    printf("| Code | Name                      | Price   | Stock | Disc%%  | Category          | Subcategory        |\n");
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
    for (int i = 0; i < n; ++i) {
        int mc = (strcmp(cat, "all")==0 || strlen(cat)==0) ? 1 : (strcmp(products[i].category, cat)==0);
        int ms = (strcmp(sub, "all")==0 || strlen(sub)==0) ? 1 : (strcmp(products[i].subcategory, sub)==0);
        if (mc && ms) {
            printf("| %-4d | %-25s | %7.2f | %5d | %5.1f | %-17s | %-17s |\n",
                   products[i].code, products[i].name, products[i].price, products[i].stock,
                   products[i].discount, products[i].category, products[i].subcategory);
            cnt++;
        }
    }
    printf("+------+---------------------------+---------+-------+--------+-------------------+-------------------+\n");
    if (cnt == 0) printf("No matching products.\n");
}

void admin_update_product() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    if (n == 0) { printf("No products.\n"); return; }
    int code; printf("Enter product code to update: "); if (scanf("%d", &code) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); return; }
    while(getchar()!='\n');
    int idx = -1;
    for (int i = 0; i < n; ++i) if (products[i].code == code) { idx = i; break; }
    if (idx == -1) { printf("Not found.\n"); return; }
    Product *p = &products[idx];
    printf("Updating %d: %s\n", p->code, p->name);
    printf("New name (blank to keep): "); char tmp[128]; fgets(tmp, sizeof(tmp), stdin); trimnewline(tmp); if (strlen(tmp)) strncpy(p->name, tmp, sizeof(p->name));
    printf("New price (-1 to keep %.2f): ", p->price); double d; if (scanf("%lf", &d)==1) { if (d >= 0) p->price = d; } while(getchar()!='\n');
    printf("New stock (-1 to keep %d): ", p->stock); int si; if (scanf("%d", &si)==1) { if (si >= 0) p->stock = si; } while(getchar()!='\n');
    printf("New discount (-1 to keep %.2f): ", p->discount); if (scanf("%lf", &d)==1) { if (d >= 0) p->discount = d; } while(getchar()!='\n');
    printf("New category (blank to keep %s): ", p->category); fgets(tmp, sizeof(tmp), stdin); trimnewline(tmp); if (strlen(tmp)) strncpy(p->category, tmp, sizeof(p->category));
    printf("New subcategory (blank to keep %s): ", p->subcategory); fgets(tmp, sizeof(tmp), stdin); trimnewline(tmp); if (strlen(tmp)) strncpy(p->subcategory, tmp, sizeof(p->subcategory));
    if (!save_products(products, n)) printf("Save failed.\n"); else printf("Product updated.\n");
}

void admin_delete_product() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    if (n == 0) { printf("No products.\n"); return; }
    int code; printf("Enter product code to delete: "); if (scanf("%d", &code) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); return; }
    while(getchar()!='\n');
    int idx = -1;
    for (int i = 0; i < n; ++i) if (products[i].code == code) { idx = i; break; }
    if (idx == -1) { printf("Not found.\n"); return; }
    for (int i = idx; i < n-1; ++i) products[i] = products[i+1];
    n--;
    if (!save_products(products, n)) printf("Failed to save.\n"); else printf("Deleted.\n");
}

void admin_low_stock_alerts() {
    Product products[MAX_PRODUCTS];
    int n = load_products(products, MAX_PRODUCTS);
    int found = 0;
    printf("\nLow-stock products (stock < 5):\n");
    for (int i = 0; i < n; ++i) {
        if (products[i].stock < 5) {
            printf("%4d | % -20s | stock: %d\n", products[i].code, products[i].name, products[i].stock);
            found++;
        }
    }
    if (!found) printf("No low-stock products.\n");
}

/* ---------- Billing functions (cashier) ---------- */

void ensure_bills_dir() {
#ifdef _WIN32
    CreateDirectoryA(BILLS_DIR, NULL);
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s 2>/dev/null", BILLS_DIR);
    system(cmd);
#endif
}

int next_receipt_id() {
    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) return 1;
    int last = 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trimnewline(line);
        if (strlen(line) == 0) continue;
        int id = 0;
        if (sscanf(line, "%d,", &id) == 1) {
            if (id > last) last = id;
        }
    }
    fclose(fp);
    return last + 1;
}

void append_receipt_items(CartItem cart[], int cartCount, const char *customerName, const char *iso) {
    FILE *fp = fopen(RECEIPTS_FILE, "a");
    if (!fp) { printf("Warning: cannot append to %s\n", RECEIPTS_FILE); return; }
    int rid = next_receipt_id();
    for (int i = 0; i < cartCount; ++i) {
        fprintf(fp, "%d,%s,%s,%d,%s,%d,%.2f,%.2f\n",
                rid, customerName, iso, cart[i].code, cart[i].name, cart[i].qty, cart[i].priceAfterDisc, cart[i].total);
    }
    fclose(fp);
}

void append_sales_items(CartItem cart[], int cartCount, const char *iso) {
    FILE *fp = fopen(SALES_ITEMS_FILE, "a");
    if (!fp) { printf("Warning: cannot append to %s\n", SALES_ITEMS_FILE); return; }
    for (int i = 0; i < cartCount; ++i) {
        fprintf(fp, "%d,%s,%d,%.2f,%.2f,%s\n",
                cart[i].code, cart[i].name, cart[i].qty, cart[i].priceAfterDisc, cart[i].total, iso);
    }
    fclose(fp);
}

void billing_print_steady(CartItem cart[], int cartCount, const char *customerName) {
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char iso[32]; strftime(iso, sizeof(iso), "%Y-%m-%d %H:%M:%S", lt);

    printf("\n-------------------- STEADY RECEIPT --------------------\n");
    printf("Customer: %s\n", customerName);
    printf("Date: %s\n", iso);
    printf("--------------------------------------------------------\n");
    printf("%-6s %-22s %5s %10s %10s\n", "Code", "Item", "Qty", "Unit", "Subtotal");
    printf("--------------------------------------------------------\n");
    double subtotal = 0.0;
    for (int i = 0; i < cartCount; ++i) {
        printf("%-6d %-22s %5d %10.2f %10.2f\n",
               cart[i].code, cart[i].name, cart[i].qty, cart[i].priceAfterDisc, cart[i].total);
        subtotal += cart[i].total;
    }
    printf("--------------------------------------------------------\n");
    double discount = 0.0;
    double net = subtotal - discount;
    printf("%52s %10.2f\n", "Subtotal:", subtotal);
    printf("%52s %10.2f\n", "Discount:", discount);
    printf("%52s %10.2f\n", "Net Total:", net);
    printf("--------------------------------------------------------\n");
}

void billing_finalize_and_save(CartItem cart[], int cartCount, const char *customerName) {
    if (cartCount == 0) { printf("Cart empty. Nothing to finalize.\n"); return; }
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char iso[32]; strftime(iso, sizeof(iso), "%Y-%m-%d %H:%M:%S", lt);
    double subtotal = 0.0;
    for (int i = 0; i < cartCount; ++i) subtotal += cart[i].total;
    double discount = 0.0;
    double net = subtotal - discount;

    ensure_bills_dir();
    char filename[256];
    strftime(filename, sizeof(filename), BILLS_DIR "/bill_%Y%m%d_%H%M%S.txt", lt);
    FILE *bf = fopen(filename, "w");
    if (!bf) {
        printf("Failed to create bill file. Showing on console only.\n");
    } else {
        fprintf(bf, "==================== CODE_FUSION STORE BILL ====================\n");
        fprintf(bf, "Date: %s\n", iso);
        fprintf(bf, "Customer: %s\n", customerName);
        fprintf(bf, "-----------------------------------------------------\n");
        fprintf(bf, "%-6s %-22s %5s %10s %10s\n", "Code", "Item", "Qty", "Unit", "Subtotal");
        fprintf(bf, "-----------------------------------------------------\n");
        for (int i = 0; i < cartCount; ++i) {
            fprintf(bf, "%-6d %-22s %5d %10.2f %10.2f\n",
                    cart[i].code, cart[i].name, cart[i].qty, cart[i].priceAfterDisc, cart[i].total);
        }
        fprintf(bf, "-----------------------------------------------------\n");
        fprintf(bf, "%52s %10.2f\n", "Subtotal:", subtotal);
        fprintf(bf, "%52s %10.2f\n", "Discount:", discount);
        fprintf(bf, "%52s %10.2f\n", "Net Total:", net);
        fprintf(bf, "=====================================================\n");
        fprintf(bf, " THANK YOU! VISIT AGAIN\n");
        fprintf(bf, "=====================================================\n");
        fclose(bf);
        printf("Bill saved to: %s\n", filename);
    }

    append_receipt_items(cart, cartCount, customerName, iso);
    append_sales_items(cart, cartCount, iso);

    Product products[MAX_PRODUCTS]; int n = load_products(products, MAX_PRODUCTS);
    for (int i = 0; i < cartCount; ++i) {
        Product *p = find_product_by_code(products, n, cart[i].code);
        if (p) {
            p->stock -= cart[i].qty;
            if (p->stock < 0) p->stock = 0;
        }
    }
    if (!save_products(products, n)) printf("Warning: could not update products file after sale.\n");

    printf("Checkout complete. Net Total = %.2f\n", net);
}

void billing_menu() {
    Product products[MAX_PRODUCTS];
    int prodCount = load_products(products, MAX_PRODUCTS);
    if (prodCount == 0) {
        printf("No products available. Ask admin to add products first.\n");
        return;
    }

    CartItem cart[MAX_CART];
    int cartCount = 0;
    char customerName[128];
    printf("Billing mode - enter customer name (or 'walkin'): ");
    fgets(customerName, sizeof(customerName), stdin); trimnewline(customerName);
    if (strlen(customerName) == 0) strcpy(customerName, "Walk-in");

    while (1) {
        printf("\n==== BILLING SECTION(Cashier) ====\n");
        printf("1. Show Products\n");
        printf("2. Search Product\n");
        printf("3. Add Item to Cart\n");
        printf("4. Remove Item from Cart\n");
        printf("5. View Steady Receipt\n");
        printf("6. Finalize / Save Bill\n");
        printf("0. Cancel & Back\n");
        printf("Enter choice: ");
        int ch;
        if (scanf("%d", &ch) != 1) { while(getchar()!='\n'); ch = -1; }
        while (getchar()!='\n');

        if (ch == 0) {
            printf("Exiting billing. Any unsaved cart will be lost.\n");
            return;
        }
        else if (ch == 1) {
            prodCount = load_products(products, MAX_PRODUCTS);
            printf("\nCode | Name | Price | Stock | Disc\n");
            printf("-------------------------------------------\n");
            for (int i = 0; i < prodCount; ++i) {
                printf("%4d | % -25s | %7.2f | %4d | %4.1f%%\n",
                       products[i].code, products[i].name, products[i].price, products[i].stock, products[i].discount);
            }
            pause_console();
        }
        else if (ch == 2) {
            char term[128]; printf("Enter product id or name: ");
            fgets(term, sizeof(term), stdin); trimnewline(term);
            int all_digits = 1; for (size_t i=0;i<strlen(term);++i) if (!isdigit((unsigned char)term[i])) { all_digits = 0; break; }
            if (all_digits && strlen(term)>0) {
                int id = atoi(term);
                Product *p = find_product_by_code(products, prodCount, id);
                if (p) printf("Found: %d | %s | %.2f | stock %d\n", p->code, p->name, p->price, p->stock);
                else printf("Not found.\n");
            } else {
                char lower[128]; strncpy(lower, term, sizeof(lower)); strtolower(lower);
                int found = 0;
                for (int i=0;i<prodCount;++i) {
                    char nameLower[128]; strncpy(nameLower, products[i].name, sizeof(nameLower)); strtolower(nameLower);
                    if (strstr(nameLower, lower)) {
                        printf("%d | %s | %.2f | stock %d\n", products[i].code, products[i].name, products[i].price, products[i].stock);
                        found = 1;
                    }
                }
                if (!found) printf("No matches.\n");
            }
            pause_console();
        }
        else if (ch == 3) {
            int code, qty;
            printf("Enter product code to add: ");
            if (scanf("%d", &code) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); continue;}
            printf("Enter quantity: ");
            if (scanf("%d", &qty) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); continue;}
            while(getchar()!='\n');
            Product *p = find_product_by_code(products, prodCount, code);
            if (!p) { printf("Product not found.\n"); continue; }
            if (qty <= 0) { printf("Quantity must be positive.\n"); continue; }
            if (p->stock < qty) { printf("Insufficient stock (available %d).\n", p->stock); continue; }
            double priceAfter = p->price * (100.0 - p->discount) / 100.0;
            double subtotal = priceAfter * qty;
            int foundIdx = -1;
            for (int i = 0; i < cartCount; ++i) if (cart[i].code == code) { foundIdx = i; break; }
            if (foundIdx >= 0) {
                cart[foundIdx].qty += qty;
                cart[foundIdx].priceAfterDisc = priceAfter;
                cart[foundIdx].total = cart[foundIdx].qty * cart[foundIdx].priceAfterDisc;
            } else {
                if (cartCount < MAX_CART) {
                    cart[cartCount].code = code;
                    strncpy(cart[cartCount].name, p->name, sizeof(cart[cartCount].name));
                    cart[cartCount].qty = qty;
                    cart[cartCount].priceAfterDisc = priceAfter;
                    cart[cartCount].total = subtotal;
                    cartCount++;
                } else {
                    printf("Cart full.\n");
                }
            }
            billing_print_steady(cart, cartCount, customerName);
        }
        else if (ch == 4) {
            int code; printf("Enter product code to remove from cart: ");
            if (scanf("%d", &code) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); continue;}
            while(getchar()!='\n');
            int idx = -1;
            for (int i = 0; i < cartCount; ++i) if (cart[i].code == code) { idx = i; break; }
            if (idx == -1) { printf("Not in cart.\n"); continue; }
            for (int i = idx; i < cartCount-1; ++i) cart[i] = cart[i+1];
            cartCount--;
            printf("Removed from cart.\n");
            billing_print_steady(cart, cartCount, customerName);
        }
        else if (ch == 5) {
            billing_print_steady(cart, cartCount, customerName);
            pause_console();
        }
        else if (ch == 6) {
            billing_finalize_and_save(cart, cartCount, customerName);
            cartCount = 0;
            pause_console();
            return;
        }
        else {
            printf("Invalid.\n");
        }
    }
}

/* ---------- Reports ---------- */

void report_menu() {
    while (1) {
        printf("\n==== REPORT MENU ====\n");
        printf("1. Total income (all-time)\n");
        printf("2. Daily income\n");
        printf("3. Monthly income\n");
        printf("4. Product-wise sales\n");
        printf("5. Top-selling products\n");
        printf("0. Back\n");
        printf("Enter choice: ");
        int ch; if (scanf("%d", &ch) != 1) { while(getchar()!='\n'); ch=-1; }
        while(getchar()!='\n');
        if (ch == 0) return;
        switch (ch) {
            case 1: report_total_income(); break;
            case 2: report_daily_income(); break;
            case 3: report_monthly_income(); break;
            case 4: report_product_wise(); break;
            case 5: report_top_selling(); break;
            default: printf("Invalid.\n");
        }
        pause_console();
    }
}

void report_total_income() {
    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) { printf("No receipts found.\n"); return; }
    double sum = 0.0; char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trimnewline(line);
        if (strlen(line)==0) continue;
        int rid, code, qty; char cust[128], iso[64], name[128]; double unit, subtotal;
        if (sscanf(line, "%d,%127[^,],%63[^,],%d,%127[^,],%d,%lf,%lf",
                   &rid, cust, iso, &code, name, &qty, &unit, &subtotal) == 8) {
            sum += subtotal;
        }
    }
    fclose(fp);
    printf("Total income (all time): %.2f\n", sum);
}

void report_daily_income() {
    char date[16];
    printf("Enter date (YYYY-MM-DD): ");
    fgets(date, sizeof(date), stdin); trimnewline(date);
    if (strlen(date)==0) { printf("Invalid.\n"); return; }
    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) { printf("No receipts.\n"); return; }
    double sum = 0.0; char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        int rid, code, qty; char cust[128], iso[64], name[128]; double unit, subtotal;
        if (sscanf(line, "%d,%127[^,],%63[^,],%d,%127[^,],%d,%lf,%lf",
                   &rid, cust, iso, &code, name, &qty, &unit, &subtotal) == 8) {
            if (strncmp(iso, date, strlen(date)) == 0) sum += subtotal;
        }
    }
    fclose(fp);
    printf("Total income on %s : %.2f\n", date, sum);
}

void report_monthly_income() {
    char yearmon[8];
    printf("Enter month (YYYY-MM): ");
    fgets(yearmon, sizeof(yearmon), stdin); trimnewline(yearmon);
    if (strlen(yearmon) < 7) { printf("Invalid.\n"); return; }
    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) { printf("No receipts.\n"); return; }
    double sum = 0.0; char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        int rid, code, qty; char cust[128], iso[64], name[128]; double unit, subtotal;
        if (sscanf(line, "%d,%127[^,],%63[^,],%d,%127[^,],%d,%lf,%lf",
                   &rid, cust, iso, &code, name, &qty, &unit, &subtotal) == 8) {
            if (strncmp(iso, yearmon, 7) == 0) sum += subtotal;
        }
    }
    fclose(fp);
    printf("Total income in %s: %.2f\n", yearmon, sum);
}

void report_product_wise() {
    int pid;
    printf("Enter product code: ");
    if (scanf("%d", &pid) != 1) { while(getchar()!='\n'); printf("Invalid.\n"); return; }
    while(getchar()!='\n');
    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) { printf("No receipts.\n"); return; }
    char line[MAX_LINE];
    double total = 0.0; int qtySum = 0;
    while (fgets(line, sizeof(line), fp)) {
        int rid, code, qty; char cust[128], iso[64], name[128]; double unit, subtotal;
        if (sscanf(line, "%d,%127[^,],%63[^,],%d,%127[^,],%d,%lf,%lf",
                   &rid, cust, iso, &code, name, &qty, &unit, &subtotal) == 8) {
            if (code == pid) { total += subtotal; qtySum += qty; printf("%s | %s | qty %d | subtotal %.2f\n", iso, name, qty, subtotal); }
        }
    }
    fclose(fp);
    printf("Total sold qty: %d | Total revenue: %.2f\n", qtySum, total);
}

int compare_top(const void *a, const void *b) {
    const long *A = a;
    const long *B = b;
    if (A[2] != B[2]) return (B[2] - A[2]) > 0 ? 1 : -1;
    if (A[3] > B[3]) return -1;
    if (A[3] < B[3]) return 1;
    return 0;
}

void report_top_selling() {
    FILE *fp = fopen(SALES_ITEMS_FILE, "r");
    if (!fp) { printf("No sales items.\n"); return; }
    typedef struct { int code; char name[128]; long qty; double revenue; } Agg;
    Agg agg[1000]; int aggc = 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trimnewline(line);
        if (strlen(line)==0) continue;
        int code, qty; char name[128], date[64]; double price, subtotal;
        if (sscanf(line, "%d,%127[^,],%d,%lf,%lf,%63[^\n]", &code, name, &qty, &price, &subtotal, date) == 6) {
            int idx = -1;
            for (int i=0;i<aggc;++i) if (agg[i].code == code) { idx = i; break; }
            if (idx == -1) {
                agg[aggc].code = code; strncpy(agg[aggc].name, name, sizeof(agg[aggc].name)); agg[aggc].qty = qty; agg[aggc].revenue = subtotal; aggc++;
            } else {
                agg[idx].qty += qty; agg[idx].revenue += subtotal;
            }
        }
    }
    fclose(fp);
    if (aggc == 0) { printf("No items sold yet.\n"); return; }
    for (int i=0;i<aggc-1;++i) for (int j=i+1;j<aggc;++j) {
        if (agg[j].qty > agg[i].qty || (agg[j].qty == agg[i].qty && agg[j].revenue > agg[i].revenue)) {
            Agg tmp = agg[i]; agg[i] = agg[j]; agg[j] = tmp;
        }
    }
    int top = aggc < 10 ? aggc : 10;
    printf("Top %d selling products:\n", top);
    printf("Rank | Code | Qty Sold | Revenue | Name\n");
    printf("---------------------------------------------------------\n");
    for (int i=0;i<top;++i) {
        printf("%4d | %4d | %8ld | %8.2f | %s\n", i+1, agg[i].code, agg[i].qty, agg[i].revenue, agg[i].name);
    }
}

/* ---------- Customer Management Prototypes ---------- */
void customer_menu();
void register_customer();
void update_customer();
void search_customer();
void fetch_receipt_history();

/* ---------- Customer Management Implementation ---------- */
int next_customer_id() {
    FILE *fp = fopen("customers.txt", "r");
    int id = 1, last = 0;
    if (fp) {
        while (fscanf(fp, "%d,%*[^,],%*[^,],%*[^,],%*[^,\n]\n", &id) == 1)
            last = id;
        fclose(fp);
    }
    return last + 1;
}

void register_customer() {
    FILE *fp = fopen("customers.txt", "a");
    if (!fp) { printf("Could not open customers.txt\n"); return; }
    Customer c;
    c.id = next_customer_id();
    printf("Enter Name: "); fgets(c.name, sizeof(c.name), stdin); trimnewline(c.name);
    printf("Enter Phone: "); fgets(c.phone, sizeof(c.phone), stdin); trimnewline(c.phone);
    printf("Enter Email: "); fgets(c.email, sizeof(c.email), stdin); trimnewline(c.email);
    printf("Enter Address: "); fgets(c.address, sizeof(c.address), stdin); trimnewline(c.address);
    fprintf(fp, "%d,%s,%s,%s,%s\n", c.id, c.name, c.phone, c.email, c.address);
    fclose(fp);
    printf("Customer registered.\n");
}

void update_customer() {
    char search[50], nameLower[50], custNameLower[50];
    int found = 0, updateChoice;
    printf("Enter customer name to update: ");
    fgets(search, sizeof(search), stdin); trimnewline(search);
    strncpy(nameLower, search, sizeof(nameLower)); strtolower(nameLower);

    FILE *fp = fopen("customers.txt", "r");
    FILE *temp = fopen("customers_temp.txt", "w");
    if (!fp || !temp) { printf("File error.\n"); return; }

    Customer c;
    while (fscanf(fp, "%d,%49[^,],%14[^,],%49[^,],%99[^\n]\n", &c.id, c.name, c.phone, c.email, c.address) == 5) {
        strncpy(custNameLower, c.name, sizeof(custNameLower)); strtolower(custNameLower);
        if (strcmp(nameLower, custNameLower) == 0) {
            found = 1;
            printf("What do you want to update?\n");
            printf("1. Phone\n2. Email\n3. Address\nEnter choice: ");
            scanf("%d", &updateChoice); while(getchar()!='\n');
            switch (updateChoice) {
                case 1:
                    printf("Enter new phone: "); fgets(c.phone, sizeof(c.phone), stdin); trimnewline(c.phone); break;
                case 2:
                    printf("Enter new email: "); fgets(c.email, sizeof(c.email), stdin); trimnewline(c.email); break;
                case 3:
                    printf("Enter new address: "); fgets(c.address, sizeof(c.address), stdin); trimnewline(c.address); break;
                default:
                    printf("Invalid choice. No update performed.\n");
            }
            printf("Customer updated.\n");
        }
        fprintf(temp, "%d,%s,%s,%s,%s\n", c.id, c.name, c.phone, c.email, c.address);
    }
    fclose(fp); fclose(temp);
    remove("customers.txt");
    rename("customers_temp.txt", "customers.txt");
    if (!found) printf("Customer not found.\n");
}

void search_customer() {
    char search[50], nameLower[50], phoneLower[15], custNameLower[50], custPhoneLower[15];
    int found = 0, searchId = 0, isId = 0;
    printf("Enter customer name, ID, or phone to search: ");
    fgets(search, sizeof(search), stdin); trimnewline(search);

    if (sscanf(search, "%d", &searchId) == 1) isId = 1;
    strncpy(nameLower, search, sizeof(nameLower)-1); nameLower[sizeof(nameLower)-1] = '\0'; strtolower(nameLower);
    strncpy(phoneLower, search, sizeof(phoneLower)-1); phoneLower[sizeof(phoneLower)-1] = '\0'; strtolower(phoneLower);

    FILE *fp = fopen("customers.txt", "r");
    if (!fp) { printf("File error.\n"); return; }
    Customer c;
    char line[512];
    printf("Results:\n");
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    printf("| ID | Name                 | Phone         | Email                     | Address                  |\n");
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    while (fgets(line, sizeof(line), fp)) {
        trimnewline(line);
        memset(&c, 0, sizeof(c));
        int fields = sscanf(line, "%d,%49[^,],%14[^,],%49[^,],%99[^\n]", &c.id, c.name, c.phone, c.email, c.address);
        if (fields < 2) continue;
        strncpy(custNameLower, c.name, sizeof(custNameLower)-1); custNameLower[sizeof(custNameLower)-1] = '\0'; strtolower(custNameLower);
        strncpy(custPhoneLower, c.phone, sizeof(custPhoneLower)-1); custPhoneLower[sizeof(custPhoneLower)-1] = '\0'; strtolower(custPhoneLower);
        if ((isId && c.id == searchId) ||
            (strlen(nameLower) && strstr(custNameLower, nameLower)) ||
            (strlen(phoneLower) && strstr(custPhoneLower, phoneLower))) {
            printf("| %-2d | %-20s | %-13s | %-25s | %-24s |\n", c.id, c.name, c.phone, c.email, c.address);
            found = 1;
        }
    }
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    fclose(fp);
    if (!found) printf("Customer not found.\n");
}

void fetch_receipt_history() {
    char search[50], nameLower[50], receiptNameLower[50];
    int found = 0, rid, itemId, qty;
    char cname[50], date[32], itemName[128];
    double price, itemTotal;
    printf("Enter customer name to fetch receipt history: ");
    fgets(search, sizeof(search), stdin); trimnewline(search);
    strncpy(nameLower, search, sizeof(nameLower)); strtolower(nameLower);

    FILE *fp = fopen(RECEIPTS_FILE, "r");
    if (!fp) { printf("File error.\n"); return; }
    printf("Receipts:\n");
    while (fscanf(fp, "%d,%49[^,],%31[^,],%d,%127[^,],%d,%lf,%lf\n", &rid, cname, date, &itemId, itemName, &qty, &price, &itemTotal) == 8) {
        strncpy(receiptNameLower, cname, sizeof(receiptNameLower)); strtolower(receiptNameLower);
        if (strcmp(nameLower, receiptNameLower) == 0) {
            printf("ReceiptID: %d | Date: %s | Item: %s | Qty: %d | Price: %.2f | Total: %.2f\n",
                   rid, date, itemName, qty, price, itemTotal);
            found = 1;
        }
    }
    fclose(fp);
    if (!found) printf("No receipts found for this customer.\n");
}

void customer_menu() {
    int choice;
    while (1) {
        printf("\n--- Customer Management ---\n");
        printf("1. Register Customer\n");
        printf("2. Update Customer\n");
        printf("3. Search Customer\n");
        printf("4. View Customers\n");
        printf("5. Fetch Receipt History\n");
        printf("0. Back\nChoice: ");
        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); choice = -1; }
        while(getchar()!='\n');
        switch (choice) {
            case 1: register_customer(); break;
            case 2: update_customer(); break;
            case 3: search_customer(); break;
            case 4: view_customers(); break;
            case 5: fetch_receipt_history(); break;
            case 0: return;
            default: printf("Invalid choice!\n");
        }
    }
}

void print_steady_bill_top(CartItem cart[], int cartCount, const char *customerName) {
    billing_print_steady(cart, cartCount, customerName);
}

int main(void) {
    ensure_bills_dir();
    while (1) {
        clear_console();
        printf("=================================\n");
        printf("   CODE_FUSION RETAIL SYSTEM\n");
        printf("=================================\n");
        printf("Choose role:\n");
        printf("1. Store Person (Admin / Inventory)\n");
        printf("2. Billing Person (Cashier)\n");
        printf("3. Customer Management\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        int role;
        if (scanf("%d", &role) != 1) { while(getchar()!='\n'); role = -1; }
        while(getchar()!='\n');
        if (role == 0) { printf("Bye!\n"); break; }
        else if (role == 1) admin_menu();
        else if (role == 2) billing_menu();
        else if (role == 3) customer_menu();
        else printf("Invalid.\n");
    }
    return 0;
}

void pause_console() {
#ifdef _WIN32
    system("pause");
#else
    printf("Press Enter to continue...");
    getchar();
#endif
}

void view_customers() {
    FILE *fp = fopen("customers.txt", "r");
    if (!fp) { printf("No customers found.\n"); return; }
    Customer c;
    int found = 0;
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    printf("| ID | Name                 | Phone         | Email                     | Address                  |\n");
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    while (fscanf(fp, "%d,%49[^,],%14[^,],%49[^,],%99[^\n]\n", &c.id, c.name, c.phone, c.email, c.address) == 5) {
        printf("| %-2d | %-20s | %-13s | %-25s | %-24s |\n", c.id, c.name, c.phone, c.email, c.address);
        found = 1;
    }
    printf("+----+----------------------+---------------+---------------------------+--------------------------+\n");
    fclose(fp);
    if (!found) printf("No customers found.\n");
}
