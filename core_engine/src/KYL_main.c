#include <string.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

// ==========================================
// ZERO-TRUST ENGINE INTEGRATION
// ==========================================
// Note: Adjust these include paths based on exactly where your KYL_main.c is located.
// If it is in api_gateway/src/, use "../../core_engine/include/pager.h"
// If it is in core_engine/src/, use "../include/pager.h"
#include "../../core_engine/include/pager.h"
#include "../../core_engine/include/buffer_pool.h"
#include "../../core_engine/include/bplus_tree.h"

#define MAX_LINE_LENGTH 256
#define SHIFT 3  // since Caesar Cipher shift value is 3
#define HISTORY_FILE "passwords_history.txt"
#define MASTER_PASSWORD_FILE "master_password.txt"
#define LOG_FILE "activity_log.txt"

// Converts a string key (like "twitter") into a unique uint32_t integer for the B+ Tree.
uint32_t hash_string_to_key(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

void print_usage() {
    printf("Usage: KYL <command> [options]\n");
    printf("Commands:\n");
    printf("  add <service> <password>         Add a new password\n");
    printf("  get <service>                    Retrieve a stored password\n");
    printf("  delete <service> <password>      Delete a stored password\n");
    printf("  update <service> <new_password>  Update a stored password\n");
    printf("  generate <length>                Generate a random password\n");
    printf("  Check <password>                 Checks if your password is breached or not\n");
}


const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char lower[] = "abcdefghijklmnopqrstuvwxyz";
const char numbers[] = "0123456789";
const char special[] = "!@#$%^&*()_-+=<>?";


void log_activity(const char *action, const char *service) {
    FILE *logFile = fopen(LOG_FILE, "a");
    if (logFile == NULL) {
        printf("Error: Unable to open log file.\n");
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    
    fprintf(logFile, "[%s] Action: %s, Service: %s\n", timestamp, action, service);
    fclose(logFile);
}

void display_welcome_pattern() {
    printf("\n======================================\n");
    printf("   WELCOME TO KYL :- THE PASSWORD MANAGER\n");
    printf("   (POWERED BY ZERO-TRUST B+ TREE ENGINE)\n");
    printf("======================================\n");
    printf(" ╭╮╭━╮╱╱╱╭╮ \n");
    printf(" ┃┃┃╭╯╱╱╱┃┃\n");
    printf(" ┃╰╯╯╭╮╱╭┫┃\n ");
    printf(" ┃╭╮┃┃┃╱┃┃┃╱╭╮\n");
    printf(" ┃┃┃╰┫╰━╯┃╰━╯┃\n");
    printf(" ╰╯╰━┻━╮╭┻━━━╯\n");
    printf(" ╱╱╱╱╭━╯┃\n");
    printf(" ╱╱╱╱╰━━╯\n");
    printf("======================================\n");
    printf("Your password manager is ready to use!\n");
    printf("======================================\n\n");
    printf("Created by: Kedar Dixit\n");
    printf("======================================\n\n");
}


// Function to hash a given password using SHA-256
void hash_password(const char *password, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);

    // Convert hash to hexadecimal string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[SHA256_DIGEST_LENGTH * 2] = '\0';
}

// Function to hide password 
void mask_password_input(char *password) {
    struct termios oldt, newt;
    int i = 0;
    char ch;

    // disabling echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Read password character by character
    while ((ch = getchar()) != '\n' && ch != EOF && i < 49) {
        password[i++] = ch;
    }
    password[i] = '\0';

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

// Function to check if a master password is set
int is_master_password_set() {
    FILE *file = fopen(MASTER_PASSWORD_FILE, "r");
    if (file) {
        fclose(file);
        return 1;  
    }
    return 0; 
}

// to set a master password
void set_master_password() {
    char master_password[50], hashed_password[SHA256_DIGEST_LENGTH * 2 + 1];

    printf("No master password set. Please create a master password: ");
    mask_password_input(master_password);

    // Hash the master password and store it in the file
    hash_password(master_password, hashed_password);

    FILE *file = fopen(MASTER_PASSWORD_FILE, "w");
    if (file) {
        fprintf(file, "%s\n", hashed_password);
        fclose(file);
        printf("Master password set successfully.\n");
        display_welcome_pattern();
    } else {
        printf("Error: Unable to set master password.\n");
    }
}


int verify_master_password() {
    char input_password[50], hashed_input[SHA256_DIGEST_LENGTH * 2 + 1], stored_hash[SHA256_DIGEST_LENGTH * 2 + 1];

    printf("Enter master password: ");
    mask_password_input(input_password);

    // Hash the entered password
    hash_password(input_password, hashed_input);

  
    FILE *file = fopen(MASTER_PASSWORD_FILE, "r");
    if (!file) {
        printf("Error: Unable to read master password file.\n");
        return 0;
    }

    fscanf(file, "%s", stored_hash);
    fclose(file);

    // Compare the entered hash with the stored hash
    if (strcmp(hashed_input, stored_hash) == 0) {
        return 1;  
    } else {
        printf("Incorrect master password.\n");
        return 0;
    }
}

// Convert the SHA-156 hash to a hexadecimal string
void sha1_to_hex(const unsigned char *hash, char *output) {
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
}

//to collect data from the cURL request
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
    strcat((char *)data, (char *)ptr);
    return size * nmemb;
}

// Function to check password 
int is_password_pwned(const char *password) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char hash_hex[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char prefix[6] = {0};
    char api_url[128] = {0};
    char response[8192] = {0};

    // Compute SHA-1 hash of the password
    SHA1((unsigned char *)password, strlen(password), hash);
    sha1_to_hex(hash, hash_hex);

    // (k-anonymity)
    strncpy(prefix, hash_hex, 5);

  
    sprintf(api_url, "https://api.pwnedpasswords.com/range/%s", prefix);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize cURL\n");
        return 0;
    }

    // Set cURL options
    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "cURL request failed: %s\n", curl_easy_strerror(res));
        return 0;
    }

    // Check if the hash suffix exists in the response
    if (strstr(response, hash_hex + 5) != NULL) {
        return 1; // Password is pwned
    }

    return 0; // Password is not pwned
}


int check_password_strength(const char *password) {
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    int length = strlen(password);

    // minimum length is 8
    if (length < 8) {
        printf("Password must be at least 8 characters long.\n");
        return 0;
    }

    for (int i = 0; i < length; i++) {
        if (isupper(password[i])) has_upper = 1;
        else if (islower(password[i])) has_lower = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else if (ispunct(password[i])) has_special = 1;
    }
    
    if (!has_upper) printf("Password must contain at least one uppercase letter.\n");
    if (!has_lower) printf("Password must contain at least one lowercase letter.\n");
    if (!has_digit) printf("Password must contain at least one digit.\n");
    if (!has_special) printf("Password must contain at least one special character.\n");
    
    if (has_upper && has_lower && has_digit && has_special) {
        return 1; // Password is strong
    } else {
        return 0; // Password is weak
    }
}

// Store password history for a service
void log_password_history(const char *service, const char *password) {
    FILE *history_file = fopen(HISTORY_FILE, "a");
    if (!history_file) {
        printf("Error opening history file.\n");
        return;
    }
    fprintf(history_file, "Service: %s, Password: %s\n", service, password);
    fclose(history_file);
}

// Caesar Cipher 
void encrypt_caesar(char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            char offset = isupper(password[i]) ? 'A' : 'a';
            password[i] = ((password[i] - offset + SHIFT) % 26) + offset;
        }
    }
}

// Caesar Cipher decryption
void decrypt_caesar(char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            char offset = isupper(password[i]) ? 'A' : 'a';
            password[i] = ((password[i] - offset - SHIFT + 26) % 26) + offset;
        }
    }
}

// ==========================================
// ZERO-TRUST ENGINE CRUD OPERATIONS
// ==========================================

void get_password(BPlusTree *tree, const char *service) {
    uint32_t key = hash_string_to_key(service);
    char retrieved_password[MAX_VALUE_SIZE];
    memset(retrieved_password, 0, MAX_VALUE_SIZE);

    if (tree_search(tree, key, retrieved_password) == STATUS_SUCCESS) {
        if (strcmp(retrieved_password, "DELETED") == 0) {
            printf("Service '%s' not found (Has been deleted).\n", service);
        } else {
            decrypt_caesar(retrieved_password);
            printf("Password for service '%s': %s\n", service, retrieved_password);
        }
    } else {
        printf("Service '%s' not found.\n", service);
    }
}

void add_generated_password(BPlusTree *tree, const char *service, const char *password) {
    char encrypted_password[MAX_VALUE_SIZE];
    memset(encrypted_password, 0, MAX_VALUE_SIZE);
    strncpy(encrypted_password, password, MAX_VALUE_SIZE - 1);
    
    encrypt_caesar(encrypted_password);
    uint32_t key = hash_string_to_key(service);
    
    tree_insert(tree, key, encrypted_password);
    log_password_history(service, password);

    printf("Generated password added successfully for service '%s' to Zero-Trust Engine.\n", service);
}
 
void delete_password(BPlusTree *tree, const char *service, const char *password_to_delete) {
    uint32_t key = hash_string_to_key(service);
    char retrieved_password[MAX_VALUE_SIZE];
    memset(retrieved_password, 0, MAX_VALUE_SIZE);

    if (tree_search(tree, key, retrieved_password) == STATUS_SUCCESS) {
        decrypt_caesar(retrieved_password);
        if (strcmp(retrieved_password, password_to_delete) == 0) {
            // Soft delete by overwriting with a "DELETED" flag in the Tree
            tree_insert(tree, key, "DELETED");
            printf("Password deleted successfully from Engine.\n");
        } else {
            printf("Authentication failed. Incorrect password provided for deletion.\n");
        }
    } else {
        printf("Password not found.\n");
    }
}

void update_password(BPlusTree *tree, const char *service, const char *new_password) {
    if (!check_password_strength(new_password)) {
        printf("Password update failed: Weak password.\n");
        return;
    }

    char encrypted_password[MAX_VALUE_SIZE];
    memset(encrypted_password, 0, MAX_VALUE_SIZE);
    strncpy(encrypted_password, new_password, MAX_VALUE_SIZE - 1);
    
    encrypt_caesar(encrypted_password);
    uint32_t key = hash_string_to_key(service);
    
    // In our Engine, inserting a duplicate key places it in front of the old one, acting as an update
    tree_insert(tree, key, encrypted_password);

    log_password_history(service, new_password);
    printf("Password updated successfully.\n");
}


void add_password(BPlusTree *tree, const char *service, const char *password) {
    if (!check_password_strength(password)) {
        printf("Password addition failed: Weak password.\n");
        return;
    }

    char encrypted_password[MAX_VALUE_SIZE];
    memset(encrypted_password, 0, MAX_VALUE_SIZE);
    strncpy(encrypted_password, password, MAX_VALUE_SIZE - 1);
    
    encrypt_caesar(encrypted_password);
    uint32_t key = hash_string_to_key(service);

    tree_insert(tree, key, encrypted_password);
    log_password_history(service, password);

    printf("Password added successfully for service '%s' to Zero-Trust Engine.\n", service);
}

void generate_password(int length, BPlusTree *tree) {
    char password[length + 1];
    char available_chars[100];
    available_chars[0] = '\0';

    // Concatenate all character sets
    strcat(available_chars, upper);
    strcat(available_chars, lower);
    strcat(available_chars, numbers);
    strcat(available_chars, special);

    int available_len = strlen(available_chars);

    srand(time(NULL));

    for (int i = 0; i < length; i++) {
        password[i] = available_chars[rand() % available_len];
    }

    password[length] = '\0';  
    printf("Generated password: %s\n", password);

    char choice;
    printf("Do you want to save this password? (y/n): ");
    scanf(" %c", &choice);

    if (tolower(choice) == 'y') {
        char service[50];
        printf("Enter the service name for this password: ");
        scanf("%s", service);

        add_generated_password(tree, service, password);
    }
}

// ==========================================
// MAIN ENTRY POINT
// ==========================================
int main(int argc, char *argv[]) {
    // 1. Authenticate user before booting the secure engine
    if(!is_master_password_set()){
        set_master_password();
    } else {
        if(!verify_master_password()){
            printf("Access denied.\n");
            return 1;
        }
    } 

    if (argc < 2) {
        printf("Error: No command provided.\n");
        print_usage();
        return 1;
    }

    char *command = argv[1];

    // 2. Boot up the Zero-Trust Database Engine
    Pager* pager = pager_open("kyl_vault.db");
    BufferPoolManager* bpm = bpm_create(pager);
    BPlusTree* tree = tree_create(bpm, 0);

    // 3. Command Routing
    if (strcmp(command, "add") == 0) {
        if (argc != 4) {
            printf("Error: Invalid usage for 'add'.\n");
            print_usage();
        } else {
            char *service = argv[2];
            char *password = argv[3];
            add_password(tree, service, password);
            log_activity("Added Password", service);
        }
    } 
    else if (strcmp(command, "get") == 0) {
        if (argc != 3) {
            printf("Error: Invalid usage for 'get'.\n");
            print_usage();
        } else {
            char *service = argv[2];
            get_password(tree, service);
            log_activity("Fetched Password", service);
        }
    } 
    else if (strcmp(command, "delete") == 0) {
        if (argc != 4) {
            printf("Error: Invalid usage for 'delete'.\n");
            print_usage();
        } else {
            char *service = argv[2];
            char *password = argv[3];
            delete_password(tree, service, password);
            log_activity("Deleted Password", service);
        }
    } 
    else if (strcmp(command, "update") == 0) {
        if (argc != 4) {
            printf("Error: Invalid usage for 'update'.\n");
            print_usage();
        } else {
            char *service = argv[2];
            char *new_password = argv[3];
            update_password(tree, service, new_password);
            log_activity("Updated password", service);
        }
    }
    else if (strcmp(command, "generate") == 0) {
        if (argc != 3) {
            printf("Error: Invalid usage for 'generate'.\n");
            print_usage();
        } else {
            int length = atoi(argv[2]); 
            if (length <= 0) {
                printf("Error: Invalid password length.\n");
            } else {
                generate_password(length, tree);
            }
        }
    }
    else if(strcmp(command, "Check") == 0) {
        if (argc != 3){
            printf("Error: Invalid usage for 'Check'.\n");
            print_usage();
        } else {
            char *password = argv[2];
            if(is_password_pwned(password)){
                printf("Warning: this password has been compromised in data breach.\n");
            } else {
                printf("This password has not been found in any known data breach.\n");
            }
        }
    }
    else {
        printf("Error: Unknown command '%s'.\n", command);
        print_usage();
    }

    // 4. Safely shutdown Engine and flush RAM Cache to disk before exiting
    free(tree);
    bpm_destroy(bpm);
    pager_close(pager);

    return 0;
}
