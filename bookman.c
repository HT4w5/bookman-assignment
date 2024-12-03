// bookman.c: Bookstore sales management system.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Book operation flags.
#define NEW_BOOK (1 << 0)
#define DEL_BOOK (1 << 1)
#define UPD_NAME (1 << 2)
#define UPD_PRICE (1 << 3)
#define UPD_QUANT (1 << 4)
#define QRY_BOOK (1 << 5)
// Book operation return values.
#define SUCCESS 0
#define INVALID_ARG -1
#define BOOK_NONEXIST -2
#define BOOK_EXIST -3
#define MAP_INCONSIST -4

#define DEFAULT_DATA_PATH "books.dat"
#define VERSION "0.0.1"
#define MAX_CMD_LEN 256
#define MAX_LISTNAME_LEN 256
#define MAX_BOOKNAME_LEN 256
#define MAX_CMD_TOKENS 5
#define SNMAP_SIZE 10007

typedef struct SNMapNode
{
    unsigned int sn;
    struct SNMapNode *next;
} snmap_node_t;

/**
 * Hash map to query book by sn.
 */
typedef struct SNMap
{
    snmap_node_t *map[SNMAP_SIZE];
} snmap_t;

/**
 * book_t: Linked list node representing a book entry.
 * Members:
 * sn: serial number of a book, unsigned int.
 * name: book name, char pointer.
 * price: book price, unsigned int.
 * quantity: books in stock, unsigned int.
 */
typedef struct BookNode
{
    unsigned int sn;
    char *name;
    unsigned int price;
    unsigned int quantity;
    struct BookNode *next;
} book_t;

/**
 * blist_t: Linked list of books in stock.
 */
typedef struct BookList
{
    unsigned int n;
    char *name;
    book_t *head;
    snmap_t *snmap;
} blist_t;

// Universal functions.
void error_die(const char *msg);
int save_data(const blist_t *blist, const char *path);
int read_data(blist_t *blist, const char *path);

// Data structure functions.

// SN hash map.
snmap_t *snmap_create();
snmap_node_t *snmap_node_create();
void snmap_destroy(snmap_t *snmap);
unsigned int sn_hash(unsigned int sn);
void snmap_append(snmap_t *snmap, unsigned int sn);
void snmap_remove(snmap_t *snmap, unsigned int sn);
int snmap_query(snmap_t *snmap, unsigned int sn);

// Book list.
book_t *book_create();
blist_t *blist_create();
void book_destroy(book_t *book);
void blist_destroy(blist_t *blist);
int blist_op(blist_t *blist, book_t *data, int opflag);

int main(void)
{
    // Interactive shell for now.

    // TODO: custom path, parse cmdline params.
    // Print welcome message.
    printf("\n");
    printf("Welcome to bookman (%s)\n", VERSION);
    printf("\n");

    // Read data from default save location.
    blist_t *booklist = blist_create();
    if (read_data(booklist, DEFAULT_DATA_PATH))
    {
        printf("Saved data not found, new data file created\n");
        printf("\n");
    }

    printf("Input help for help\n");
    printf("\n");

    // Interactive loop.
    char buf[MAX_CMD_LEN + 1];
    char *cmd[MAX_CMD_TOKENS + 1];
    int ntoken;
    char *token;
    char exit = 0;
    while (1)
    {
        // Read command.
        printf("(%s)> ", booklist->name);
        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            error_die("Error reading command");
        }
        // Null terminate.
        buf[strcspn(buf, "\n")] = '\0';
        // Parse command.
        ntoken = 0;
        token = strtok(buf, " ");
        exit = 0;
        // Seperate command into tokens.
        while (token != NULL)
        {
            cmd[ntoken] = token;
            ntoken++;
            token = strtok(NULL, " ");
        }
        // Determine command.
        if (strcmp(cmd[0], "help") == 0)
        {
            // Print help message.
            printf("\nHelp:\n\n");

            printf("  Modification\n");
            printf("   add [SN] [NAME] [PRICE] [QUANTITY]        add a new entry\n");
            printf("   del [SN]                                  delete an entry\n");
            printf("   mod [name|price|quantity] [SN] [VALUE]    modify specified property of an entry\n");
            printf("   modall [SN] [NAME] [PRICE] [QUANTITY]     modify all properties of an entry\n\n");

            printf("  Query\n");
            printf("   query [name|price|quantity] [SN]          query specified property of an entry\n");
            printf("   query all [SN]                            query all properties of an entry\n");
            printf("   queryall                                  query all properties of all entries\n");
            printf("   sort [name|price] [a|d]                   sort entries by name|price in acsending|decsending order\n\n");

            printf("  Transaction\n");
            printf("   sell [SN] [QUANTITY]                      sell specified quantity of specified entry\n\n");

            printf("  Save & Exit\n");
            printf("   write                                     save modified data to file\n");
            printf("   quit                                      exit without saving\n\n");

            printf("  Misc\n");
            printf("   help                                      print help message\n\n");
        }
        else if (strcmp(cmd[0], "add") == 0) // Add book.
        {
            if (ntoken != 5)
            {
                printf("Invalid command\n");
                continue;
            }
            // Add new book.
            book_t newbook;
            newbook.name = cmd[2];

            if (sscanf(cmd[1], "%d", &newbook.sn) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            if (sscanf(cmd[3], "%d", &newbook.price) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            if (sscanf(cmd[4], "%d", &newbook.quantity) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            int opres = 0;
            opres = blist_op(booklist, &newbook, NEW_BOOK);
            // Check result.
            if (opres < 0)
            {
                if (opres == BOOK_EXIST)
                {
                    printf("Book with same SN already exists\n");
                    continue;
                }
                else if (opres == INVALID_ARG)
                {
                    printf("Invalid command\n");
                    continue;
                }
                else if (opres == MAP_INCONSIST)
                {
                    printf("Internal error\n");
                    continue;
                }
                else
                {
                    printf("Unknown error\n");
                    continue;
                }
            }
            else
            {
                printf("Success\n");
            }
        }
        else if (strcmp(cmd[0], "del") == 0) // Delete book.
        {
            if (ntoken != 2)
            {
                printf("Invalid command\n");
                continue;
            }
            book_t delbook;
            if (sscanf(cmd[1], "%d", &delbook.sn) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            int opres = 0;
            opres = blist_op(booklist, &delbook, DEL_BOOK);
            // Check result.
            if (opres < 0)
            {
                if (opres == BOOK_NONEXIST)
                {
                    printf("Book doesn't exist\n");
                    continue;
                }
                else if (opres == INVALID_ARG)
                {
                    printf("Invalid command\n");
                    continue;
                }
                else if (opres == MAP_INCONSIST)
                {
                    printf("Internal error\n");
                    continue;
                }
                else
                {
                    printf("Unknown error\n");
                    continue;
                }
            }
            else
            {
                printf("Success\n");
            }
        }
        else if (strcmp(cmd[0], "mod") == 0)
        {
            if (ntoken != 4)
            {
                printf("Invalid command\n");
                continue;
            }
            book_t modbook;
            if (sscanf(cmd[2], "%d", &modbook.sn) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            int opflag;
            if (strcmp(cmd[1], "name") == 0)
            {
                opflag = UPD_NAME;
                modbook.name = cmd[3];
            }
            else if (strcmp(cmd[1], "price") == 0)
            {
                opflag = UPD_PRICE;
                if (sscanf(cmd[3], "%d", &modbook.price) <= 0)
                {
                    printf("Invalid command\n");
                    continue;
                }
            }
            else if (strcmp(cmd[1], "quantity") == 0)
            {
                opflag = UPD_QUANT;
                if (sscanf(cmd[3], "%d", &modbook.quantity) <= 0)
                {
                    printf("Invalid command\n");
                    continue;
                }
            }
            else
            {
                printf("Invalid command\n");
                continue;
            }
            int opres = 0;
            opres = blist_op(booklist, &modbook, opflag);
            // Check result.
            if (opres < 0)
            {
                if (opres == BOOK_NONEXIST)
                {
                    printf("Book doesn't exist\n");
                    continue;
                }
                else if (opres == INVALID_ARG)
                {
                    printf("Invalid command\n");
                    continue;
                }
                else if (opres == MAP_INCONSIST)
                {
                    printf("Internal error\n");
                    continue;
                }
                else
                {
                    printf("Unknown error\n");
                    continue;
                }
            }
            else
            {
                printf("Success\n");
            }
        }
        else if (strcmp(cmd[0], "modall") == 0)
        {
            if (ntoken != 5)
            {
                printf("Invalid command\n");
                continue;
            }
            book_t modbook;
            if (sscanf(cmd[1], "%d", &modbook.sn) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }
            int opflag;
            opflag = UPD_NAME | UPD_PRICE | UPD_QUANT;

            modbook.name = cmd[2];

            if (sscanf(cmd[3], "%d", &modbook.price) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }

            if (sscanf(cmd[4], "%d", &modbook.quantity) <= 0)
            {
                printf("Invalid command\n");
                continue;
            }

            int opres = 0;
            opres = blist_op(booklist, &modbook, opflag);
            // Check result.
            if (opres < 0)
            {
                if (opres == BOOK_NONEXIST)
                {
                    printf("Book doesn't exist\n");
                    continue;
                }
                else if (opres == INVALID_ARG)
                {
                    printf("Invalid command\n");
                    continue;
                }
                else if (opres == MAP_INCONSIST)
                {
                    printf("Internal error\n");
                    continue;
                }
                else
                {
                    printf("Unknown error\n");
                    continue;
                }
            }
            else
            {
                printf("Success\n");
            }
        }
    }
}

void error_die(const char *msg)
{
    fprintf(stderr, "Fatal error: %s\n", msg);
    exit(EXIT_FAILURE);
}

// Booklist functions.

book_t *book_create()
{
    book_t *new = (book_t *)malloc(sizeof(book_t));
    if (new == NULL)
    {
        error_die("Malloc failed");
    }
    memset(new, 0, sizeof(book_t));
    return new;
}

blist_t *blist_create()
{
    blist_t *new = (blist_t *)malloc(sizeof(blist_t));
    if (new == NULL)
    {
        error_die("Malloc failed");
    }
    memset(new, 0, sizeof(blist_t));
    new->snmap = snmap_create();
    return new;
}

void book_destroy(book_t *book)
{
    free(book->name);
    free(book);
}

void blist_destroy(blist_t *blist)
{
    book_t *current = blist->head;
    book_t *temp = NULL;
    while (current != NULL)
    {
        temp = current;
        current = current->next;
        book_destroy(temp);
    }
    snmap_destroy(blist->snmap);
    free(blist);
}

int blist_op(blist_t *blist, book_t *data, int opflag)
{
    if (opflag == 0)
    {
        return INVALID_ARG;
    }
    else if (opflag & DEL_BOOK) // Delete entry.
    {
        if (opflag ^ DEL_BOOK) // Other flags present.
        {
            return INVALID_ARG;
        }
        if (snmap_query(blist->snmap, data->sn) == 0)
        {
            return BOOK_NONEXIST;
        }
        else
        {
            // Find and delete book entry.
            book_t *current = blist->head;
            book_t *last = NULL;
            if (current->sn == data->sn)
            {
                blist->head = current->next;
                book_destroy(current);
                // Remove hashmap entry.
                snmap_remove(blist->snmap, data->sn);
                blist->n--;
                return SUCCESS;
            }
            else
            {
                while (current != NULL)
                {
                    if (current->sn == data->sn)
                    {
                        // Delete entry.
                        last->next = current->next;
                        book_destroy(current);
                        // Remove hashmap entry.
                        snmap_remove(blist->snmap, data->sn);
                        blist->n--;
                        return SUCCESS;
                    }
                    last = current;
                    current = current->next;
                }
            }

            return MAP_INCONSIST;
        }
    }
    else if (opflag & NEW_BOOK)
    {
        if (opflag ^ NEW_BOOK) // Other flags present.
        {
            return INVALID_ARG;
        }
        if (snmap_query(blist->snmap, data->sn))
        {
            return BOOK_EXIST;
        }
        // Append to hashmap.
        snmap_append(blist->snmap, data->sn);
        // Construct new entry.
        book_t *newbook = book_create();
        memcpy(newbook, data, sizeof(book_t));
        // Copy name.
        newbook->name = (char *)malloc(strlen(data->name) + 1);
        strcpy(newbook->name, data->name);
        // Add to list.
        newbook->next = blist->head;
        blist->head = newbook;
        blist->n++;
        return SUCCESS;
    }
    else if (opflag & QRY_BOOK) // Query book data.
    {
        if (opflag ^ QRY_BOOK)
        {
            return INVALID_ARG;
        }
        // Query book.
        if (snmap_query(blist->snmap, data->sn) == 0)
        {
            return BOOK_NONEXIST;
        }
        // Find book.
        book_t *current = blist->head;
        while (current != NULL)
        {
            if (current->sn == data->sn) // Found.
            {
                data->price = current->price;
                data->quantity = current->quantity;
                strcpy(data->name, current->name);
                return SUCCESS;
            }
            current = current->next;
        }
        return MAP_INCONSIST;
    }
    else // Update book data.
    {
        // Query book.
        if (snmap_query(blist->snmap, data->sn) == 0)
        {
            return BOOK_NONEXIST;
        }
        // Find book location.
        book_t *current = blist->head;
        while (current != NULL)
        {
            if (current->sn == data->sn) // Found.
            {
                if (opflag & UPD_NAME)
                {
                    free(current->name);
                    current->name = (char *)malloc(strlen(data->name));
                    strcpy(current->name, data->name);
                }
                if (opflag & UPD_PRICE)
                {
                    current->price = data->price;
                }
                if (opflag & UPD_QUANT)
                {
                    current->quantity = data->quantity;
                }
                return SUCCESS;
            }
            current = current->next;
        }
        return MAP_INCONSIST;
    }
}

// SNMap functions.

unsigned int sn_hash(unsigned int sn)
{
    return sn % SNMAP_SIZE;
}

snmap_t *snmap_create()
{
    snmap_t *new = (snmap_t *)malloc(sizeof(snmap_t));
    if (new == NULL)
    {
        error_die("Malloc failed");
    }
    memset(new, 0, sizeof(snmap_t));
    return new;
}

snmap_node_t *snmap_node_create()
{
    snmap_node_t *new = (snmap_node_t *)malloc(sizeof(snmap_node_t));
    if (new == NULL)
    {
        error_die("Malloc failed");
    }
    memset(new, 0, sizeof(snmap_node_t));
    return new;
}

void snmap_destroy(snmap_t *snmap)
{
    snmap_node_t *current = snmap->map[0];
    snmap_node_t *temp = NULL;
    for (int i = 0; i < SNMAP_SIZE; ++i)
    {
        while (current != NULL)
        {
            temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(snmap);
}

void snmap_append(snmap_t *snmap, unsigned int sn)
{
    unsigned int key = sn_hash(sn);
    snmap_node_t *new = snmap_node_create();
    new->sn = sn;
    snmap_node_t *current = snmap->map[key];
    if (current != NULL)
    {
        if (current->sn == sn)
        {
            free(new);
            return;
        }
        while (current->next != NULL)
        {
            if (current->sn == sn)
            {
                free(new);
                return;
            }
            current = current->next;
        }
        current->next = new;
    }
    else
    {
        snmap->map[key] = new;
    }
}

int snmap_query(snmap_t *snmap, unsigned int sn)
{
    unsigned int key = sn_hash(sn);
    snmap_node_t *current = snmap->map[key];
    while (current != NULL)
    {
        if (current->sn == sn)
        {
            return 1;
        }
    }
    return 0;
}

void snmap_remove(snmap_t *snmap, unsigned int sn)
{
    unsigned int key = sn_hash(sn);
    snmap_node_t *current = snmap->map[key];
    snmap_node_t *last = NULL;
    // No entries.
    if (current == NULL)
    {
        return;
    }

    // First entry.
    if (current->sn == sn)
    {
        snmap->map[key] = current->next;
        free(current);
        return;
    }

    // Remaining entries.
    last = current;
    current = current->next;
    while (current != NULL)
    {
        if (current->sn == sn) // Found, remove entry.
        {
            last->next = current->next;
            free(current);
        }
        last = current;
        current = current->next;
    }
    return;
}

// File IO.
int save_data(const blist_t *blist, const char *path)
{
    errno = 0;
    FILE *datfile = fopen(path, "w");
    do
    {
        errno = 0;
        if (datfile == NULL)
        {
            perror(strerror(errno));
            perror("\n");
            break;
        }
        // Write format identifier.
        errno = 0;
        if (fprintf(datfile, "bookman %s\n", VERSION) < 0)
        {
            perror(strerror(errno));
            perror("\n");
            break;
        }
        // Write list properties.
        errno = 0;
        if (fprintf(datfile, "%s %d\n", blist->name, blist->n) < 0)
        {
            perror(strerror(errno));
            perror("\n");
            break;
        }
        // Write list data.
        char bookname[MAX_BOOKNAME_LEN + 1];
        book_t buff;
        buff.name = (char *)&bookname;
        int opres = 0;
        book_t *current = blist->head;
        while (current != NULL)
        {
            if (fprintf(datfile, "%d %s %d %d\n", current->sn, current->name, current->price, current->quantity) < 0)
            {
                perror(strerror(errno));
                perror("\n");
                perror("Error writing to file");
                if (fclose(datfile) == EOF)
                {
                    error_die("Failed to close file");
                }
                return 1;
            }
        }
        if (fclose(datfile) == EOF)
        {
            error_die("Failed to close file");
        }
        return 0;
    } while (1);

    perror("Error writing to file");
    if (fclose(datfile) == EOF)
    {
        error_die("Failed to close file");
    }
    return 1;
}

int read_data(blist_t *blist, const char *path)
{
    // Open file and determine whether file is new.
    errno = 0;
    FILE *datfile = fopen(path, "r");
    if (datfile == NULL)
    {
        if (errno == ENOENT)
        {
            /*
            errno = 0;
            datfile = fopen(path, "a+");
            if (datfile == NULL)
            {
                error_die(strerror(errno));
            }
            */
            blist->name = "NewList";
            /*
            if (fclose(datfile) == EOF)
            {
                error_die("Failed to close file");
            }
            */
            return 1;
        }
        else
        {
            error_die(strerror(errno));
        }
    }
    // Check file format.
    if (fscanf(datfile, "bookman_dat") <= 0)
    {
        error_die("Data file corrupt");
    }
    // Check version.
    if (fscanf(datfile, VERSION) <= 0)
    {
        error_die("Data file version mismatch");
    }
    // Check finished. Read data.
    char *listname = (char *)malloc(MAX_LISTNAME_LEN + 1);
    fscanf(datfile, "%s %d", listname, blist->n);
    blist->name = listname;
    // Read entries.
    book_t buff;
    int opres = 0;
    char bookname[MAX_BOOKNAME_LEN + 1];
    for (int i = 0; i < blist->n; ++i)
    {
        scanf("%d %s %d %d", &buff.sn, bookname, &buff.price, &buff.quantity);
        blist->name = (char *)&bookname;
        opres = blist_op(blist, &buff, NEW_BOOK);
        if (opres < 0)
        {
            if (opres == INVALID_ARG)
            {
                perror("Invalid error\n");
            }
            else if (opres == BOOK_EXIST)
            {
                perror("Duplicate entry");
            }
            else if (opres == MAP_INCONSIST)
            {
                perror("Hashmap inconsistent");
            }
            error_die("Load data failed");
        }
    }
    if (fclose(datfile) == EOF)
    {
        error_die("Failed to close file");
    }
    return 0;
}