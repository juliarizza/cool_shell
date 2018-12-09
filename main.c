#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>

#define COMMON_STRING_LIMIT 255
#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

// Comandos que vem por padrao na shell

int cd (char **args);
int help (char **args);
int exit_shell (char **args);

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[]) (char**) = {&cd, &help, &exit_shell};

int quantity_of_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

// Implementacao dos comandos padrao

int cd(char **args) {
    // Funcao para trocar de diretorio atual
    if (args[1] == NULL) {
        fprintf(stderr, "cool_shell: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cool_shell");
        }
    }

    return 1;
}

int help(char **args) {
    int i;
    printf("Cool Shell\n");
    printf("Digite comandos e argumentos e pressione enter.\n");
    printf("Os seguintes comandos sao padrao:\n");

    for (i = 0; i < quantity_of_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use o comando 'man' para informação adicional sobre outros programas.\n");
    return 1;
}

int exit_shell(char **args) {
    return 0;
}

// Funcoes de execucao da shell

char *read_line(void) {
    // Funcao para ler a linha enviada pelo usuario
    char *line = NULL;
    ssize_t buffer_size = 0; // getline() automaticamente aloca o buffer
    getline(&line, &buffer_size, stdin);
    return line;
}

char **split_line(char *line) {
    // Funcao para separar os argumentos da linha
    // Precisamos controlar o tamanho do buffer do array
    // e realocar se necessario um buffer maior
    int buffer_size = TOKEN_BUFFER_SIZE, position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "cool_shell: allocation error\n");
        exit(EXIT_FAILURE);
    }


    token = strtok(line, TOKEN_DELIMITERS);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "cool_shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    tokens[position] = NULL;
    return tokens;
}

int launch(char **args) {
    // Funcao para iniciar um novo processo
    pid_t pid, wpid;
    int status;

    // Usa fork() para fazer uma copia do processo pai
    pid = fork();
    if (pid == 0) {
        // Processo filho, temos que chamar exec() para
        // substituir os dados copiados pelos dados do
        // novo processo
        if (execvp(args[0], args) == -1) {
            perror("cool_shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // O comando fork() gerou um erro
        perror("cool_shell");
    } else {
        // Processo pai, continua com outras tarefas
        // ou aguarda a execucao do filho
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int execute(char **args) {
    // Funcao para executar um comando
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < quantity_of_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return launch(args);
}

void print_prompt() {
    char path[COMMON_STRING_LIMIT];
    char hostname[COMMON_STRING_LIMIT];
    struct passwd *pwd = getpwuid(getuid());

    if (getcwd(path, COMMON_STRING_LIMIT) == NULL) {
        perror("cool_shell");
        exit(EXIT_FAILURE);
    }

    if (gethostname(hostname, COMMON_STRING_LIMIT) != 0) {
        perror("cool_shell");
        exit(EXIT_FAILURE);
    }

    printf("(%s@%s) %s >>> ", pwd->pw_name, hostname, path);
}

void run() {
    char *line;
    char **args;
    int status;


    do {
        // Le, formata e executa o comando sempre
        // que uma nova linha for recebida
        print_prompt();
        line = read_line();
        args = split_line(line);
        status = execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {

    run();

    return EXIT_SUCCESS;
}
