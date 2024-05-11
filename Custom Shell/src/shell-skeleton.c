#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include  <ctype.h>
//include <Python.h>

const char *sysname = "Shellect";

enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command) {
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n",
		   command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");

	for (i = 0; i < 3; i++) {
		printf("\t\t%d: %s\n", i,
			   command->redirects[i] ? command->redirects[i] : "N/A");
	}

	printf("\tArguments (%d):\n", command->arg_count);

	for (i = 0; i < command->arg_count; ++i) {
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	}

	if (command->next) {
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command) {
	if (command->arg_count) {
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}

	for (int i = 0; i < 3; ++i) {
		if (command->redirects[i])
			free(command->redirects[i]);
	}

	if (command->next) {
		free_command(command->next);
		command->next = NULL;
	}

	free(command->name);
	free(command);
	return 0;
}

/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt() {
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}

/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command) {
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);

	// trim left whitespace
	while (len > 0 && strchr(splitters, buf[0]) != NULL) {
		buf++;
		len--;
	}

	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL) {
		// trim right whitespace
		buf[--len] = 0;
	}

	// auto-complete
	if (len > 0 && buf[len - 1] == '?') {
		command->auto_complete = true;
	}

	// background
	if (len > 0 && buf[len - 1] == '&') {
		command->background = true;
	}

	char *pch = strtok(buf, splitters);
	if (pch == NULL) {
		command->name = (char *)malloc(1);
		command->name[0] = 0;
	} else {
		command->name = (char *)malloc(strlen(pch) + 1);
		strcpy(command->name, pch);
	}

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;

	while (1) {
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// trim left whitespace
		while (len > 0 && strchr(splitters, arg[0]) != NULL) {
			arg++;
			len--;
		}

		// trim right whitespace
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) {
			arg[--len] = 0;
		}

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// piping to another command
		if (strcmp(arg, "|") == 0) {
			struct command_t *c = malloc(sizeof(struct command_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			command->next = c;
			continue;
		}

		// background process
		if (strcmp(arg, "&") == 0) {
			// handled before
			continue;
		}

		// handle input redirection
		redirect_index = -1;
		if (arg[0] == '<') {
			redirect_index = 0;
		}

		if (arg[0] == '>') {
			if (len > 1 && arg[1] == '>') {
				redirect_index = 2;
				arg++;
				len--;
			} else {
				redirect_index = 1;
			}
		}

		if (redirect_index != -1) {
			command->redirects[redirect_index] = malloc(len);
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 &&
			((arg[0] == '"' && arg[len - 1] == '"') ||
			 (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}

		command->args =
			(char **)realloc(command->args, sizeof(char *) * (arg_index + 1));

		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count = arg_index;

	// increase args size by 2
	command->args = (char **)realloc(
		command->args, sizeof(char *) * (command->arg_count += 2));

	// shift everything forward by 1
	for (int i = command->arg_count - 2; i > 0; --i) {
		command->args[i] = command->args[i - 1];
	}

	// set args[0] as a copy of name
	command->args[0] = strdup(command->name);

	// set args[arg_count-1] (last) to NULL
	command->args[command->arg_count - 1] = NULL;

	

	return 0;
}

void prompt_backspace() {
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command) {
	size_t index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one alias_line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &=
		~(ICANON |
		  ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		// handle tab
		if (c == 9) {
			buf[index++] = '?'; // autocomplete
			break;
		}

		// handle backspace
		if (c == 127) {
			if (index > 0) {
				prompt_backspace();
				index--;
			}
			continue;
		}

		if (c == 27 || c == 91 || c == 66 || c == 67 || c == 68) {
			continue;
		}

		// up arrow
		if (c == 65) {
			while (index > 0) {
				prompt_backspace();
				index--;
			}

			char tmpbuf[4096];
			printf("%s", oldbuf);
			strcpy(tmpbuf, buf);
			strcpy(buf, oldbuf);
			strcpy(oldbuf, tmpbuf);
			index += strlen(buf);
			continue;
		}

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') {
		index--;
	}

	// null terminate string
	buf[index++] = '\0';

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(struct command_t *command);

int main() {
	while (1) {
		struct command_t *command = malloc(sizeof(struct command_t));

		// set all bytes to 0
		memset(command, 0, sizeof(struct command_t));

		int code;
		code = prompt(command);
		if (code == EXIT) {
			break;
		}

		code = process_command(command);
		if (code == EXIT) {
			break;
		}

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command) {
	int r;

	if (strcmp(command->name, "") == 0) {
		return SUCCESS;
	}

	if (strcmp(command->name, "exit") == 0) {
		return EXIT;
	}

	if (strcmp(command->name, "cd") == 0) {
		if (command->arg_count > 0) {
			r = chdir(command->args[0]);
			if (r == -1) {
				printf("-%s: %s: %s\n", sysname, command->name,
					   strerror(errno));
			}

			return SUCCESS;
		}
	}


	
	pid_t pid = fork();
	// child
	if (pid == 0) {


		////////////////////// part2 ////////////////////
		// redirection >
		if (command->redirects[1] != NULL) {
            int fildir = open(command->redirects[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fildir, STDOUT_FILENO);
            close(fildir);
        }

        // redirection >>
        if (command->redirects[2] != NULL) {
            int fildir = open(command->redirects[2], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fildir, STDOUT_FILENO);
            close(fildir);
        }

        // redirection <
        if (command->redirects[0] != NULL) {
            int fildir = open(command->redirects[0], O_RDONLY);
            dup2(fildir, STDIN_FILENO);
            close(fildir);
        }



		////////////////////////////////////////////////

		/////////// Part3.1/////////////
		if (strcmp(command->name, "xdd") == 0) {
			//if gives something else than -g generate errror
			if (strcmp(command->args[1], "-g") != 0) {
				perror("wrong type of argument");
				return SUCCESS;
			}

			int group_size = atoi(command->args[2]);

			//control the group size
			if (group_size <= 0 || group_size > 16) {
				printf("Invalid group size\n");
				return SUCCESS;
			}
			
			//calculating number of columns per alias_line
			int column = 16 / group_size;
			FILE *input_stream;
			bool reading_from_stdin = false;


			//if path provided
			if (command->arg_count == 5) {
				const char *filename = command->args[3];
				input_stream = fopen(filename, "r");
				if (input_stream == NULL) {
					perror("Error opening file");
					return SUCCESS;
				}

			} 
			
			//if path is not provided stdin
			else if (command->arg_count == 4) {
				input_stream = stdin;
				reading_from_stdin = true;
			}
			
			else {
				printf("Incorrect number of arguments.\n");
				return SUCCESS;
			}

			int character = 0;
			int byte_count = 0;
			int group_count = 0;
			unsigned long long current_group = 0;

			while ((character = fgetc(input_stream)) != EOF) {
				current_group = (current_group << 8) | (unsigned char)character;
				byte_count++;

				//print the groups in hexadecimal
				if (byte_count % group_size == 0) {
					printf("%0*llx ", group_size * 2, current_group);
					current_group = 0;
					group_count++;
					if (group_count % column == 0) {
						printf("\n");
					}
				}
			}

			//remaining bytes
			if (byte_count % group_size != 0) {
				int last_bytes = byte_count % group_size;
				printf("%0*llx ", last_bytes * 2, current_group);
			}
			if (group_count % column != 0 || byte_count % group_size != 0) {
				printf("\n");
			}

			if (!reading_from_stdin) {
				fclose(input_stream);
			}

			return SUCCESS;
		}


		///////////////////////////////////

		/////////////////part3.2////////////////////////////////

		
		if (strcmp(command->name, "alias") == 0) {

			//argument check
			 if (command->arg_count < 4 || command->arg_count > 6) { 
				printf("wrong number of arguments.\n");
				return SUCCESS;
			}



			const char *alias_file = "alias_file";

			// Read the existing content of the file
			FILE *file = fopen(alias_file, "r");
			char *oldContent = NULL;
			if (file != NULL) {
				fseek(file, 0, SEEK_END);
				long length = ftell(file);
				fseek(file, 0, SEEK_SET);
				oldContent = malloc(length + 1);
				if (oldContent) {
					fread(oldContent, 1, length, file);
					oldContent[length] = '\0'; // Null-terminate the string
				}
				fclose(file);
			}

			// Open the file for writing (this will clear the file if it exists)
			file = fopen(alias_file, "w");
			if (file == NULL) {
				perror("Error opening alias file");
				free(oldContent); // Free memory if allocated
				return SUCCESS;
			}

			// Write the new alias followed by the old content
			 if (command->arg_count == 4) {
			fprintf(file, "%s,%s\n", command->args[1], command->args[2]);
			 }

			 if (command->arg_count == 5) {
			fprintf(file, "%s,%s %s\n", command->args[1], command->args[2], command->args[3]);
			 }

			 if (command->arg_count == 6) {
			fprintf(file, "%s,%s %s %s\n", command->args[1], command->args[2], command->args[3], command->args[4]);
			 }

			if (oldContent) {
				fputs(oldContent, file);
				free(oldContent);
			}

			fclose(file);
			printf("alias '%s' added.\n", command->args[1]);
			return SUCCESS;
			
	
		}

		const char *alias_file = "alias_file";
		FILE *file = fopen(alias_file, "r"); 

		if (file == NULL) {
			//perror("Error opening alias file");
			return SUCCESS;
		}

		char alias_line[1024];
		bool alias_found = false;
		char *alias_command = NULL;

		while (fgets(alias_line, sizeof(alias_line), file) != NULL) {

			char *comma = strchr(alias_line, ',');

			if (comma != NULL) {
				*comma = '\0'; 

				// I'm checking if the alias command typed in the shell if so, I'm taking the corresponding command from the file
				if (strcmp(command->name, alias_line) == 0) {
					alias_found = true;
					alias_command = comma + 1;
					break;
				}
			}
		}
		fclose(file);
		//cehcking if if there is alias and it's command
		
		if (alias_found && alias_command) {
			
			char *newline = strchr(alias_command, '\n');
			if (newline) {
				*newline = '\0';
			}

			
			char *argv[64]; 
			int argc = 0;
			char *alias_command_tok = strtok(alias_command, " ");

			while (alias_command_tok != NULL) {

				argv[argc++] = alias_command_tok;

				alias_command_tok = strtok(NULL, " ");
			}
			argv[argc] = NULL; 

			// executing the alias" command
			execvp(argv[0], argv);

			
			perror("execvp failed");
			return SUCCESS;
		} 
		else {
			
		}

		

		
		////////////////////////////////////////////////////////


		///////////////////part3.3
		if (strcmp(command->name, "good_morning") == 0) {

			if (command->arg_count == 4) {
			char *path = command->args[2];
			int minutes = atoi(command->args[1]);
		
			if (minutes < 0) {
				printf("Invalid number of minutes.\n");
				return SUCCESS;
			}

			time_t now = time(NULL);
			struct tm *timeinfo = localtime(&now);
			// adding our input minutes to the current time
			timeinfo->tm_min += minutes;
			mktime(timeinfo);

			// crearing the cron job 
			char cron_cmd[1024];
			snprintf(cron_cmd, sizeof(cron_cmd), 
					"(crontab -l 2>/dev/null; echo \"%d %d * * * mpg123 '%s' && (crontab -l | grep -v '%s' | crontab -)\" ) | crontab -", 
					timeinfo->tm_min, timeinfo->tm_hour, path, path);

			// executioning the cron job
			int status = system(cron_cmd);
			if (status != 0) {
				printf("Failed to schedule the song.\n");
			}
			else {
            printf("Alarm scheduled to play the song in %d minutes.\n", minutes);
        }
			
		} 
		else {
			printf("Incorrect number of arguments.\n");
		}
		return SUCCESS;
		}

		///////////////////////////////


		//////////////part3.4


		if (strcmp(command->name, "all_occurences") == 0) {

			// takes 3 argument
			if (command->arg_count == 5) { 
				
				//if the first argument -c it will count all_occurences
				if (strcmp(command->args[1], "-c")== 0){
					const char *word = command->args[2];
					const char *filename = command->args[3];

					FILE *file = fopen(filename, "r");
					if (file == NULL) {
						printf("Error opening file: %s\n", strerror(errno));
						return SUCCESS;
					}

					char buffer[1024];
					int word_len = strlen(word);
					int count = 0;
					while (fscanf(file, "%1023s", buffer) == 1) {
						if (strncmp(buffer, word, word_len) == 0 && (buffer[word_len] == ' ' || buffer[word_len] == '\0' || buffer[word_len] == '\n' || ispunct(buffer[word_len]))) {
							count++;
						}
				}

				printf("The word '%s' occurred %d times in the file '%s'.\n", word, count, filename);
				fclose(file);
			}

			// if the second argument is -d it will delete all occurences
			if (strcmp(command->args[1], "-d")== 0){
				const char *word = command->args[2];
				const char *filepath = command->args[3];
				FILE *input_file = fopen(filepath, "r");
				if (!input_file) {
					perror("Error opening file");
					return SUCCESS;
				}

				FILE *temp_file = tmpfile();
				if (!temp_file) {
					perror("Error opening temporary file");
					fclose(input_file);
					return SUCCESS;
				}

				char *line = NULL;
				size_t len = 0;
				ssize_t read;

				while ((read = getline(&line, &len, input_file)) != -1) {
					char *position = line;
					while ((position = strstr(position, word)) != NULL) {
						memmove(position, position + strlen(word), strlen(position + strlen(word)) + 1);
					}
					fputs(line, temp_file);
				}
				free(line);
				fclose(input_file);

				
				fseek(temp_file, 0, SEEK_SET);
				input_file = fopen(filepath, "w");
				if (!input_file) {
					perror("Error opening file for writing");
					fclose(temp_file);
					return SUCCESS;
				}
				while ((read = getline(&line, &len, temp_file)) != -1) {
					fputs(line, input_file);
				}
				free(line);
				fclose(input_file);
				fclose(temp_file); 

			}
		} 
		else {
			printf("Incorrect number of arguments. Usage: all_occurrences [word] [filename]\n");
		}
		return SUCCESS;
			}

		/////////////////////PART4////////////////////
		if (strcmp(command->name, "psvis") == 0) {


			char input1[100] = "sudo insmod module/mymodule.ko input_pid=";
			strcat(input1, command->args[1]);
			//printf("%s\n",input1);
			
			system(input1);
			

			//system("sudo dmesg | tail -n 50 > module_output.txt");

			system("sudo rmmod mymodule");

			//create_graph_from_output("module_output.txt", "process_tree.dot");

			// Generate the tree image using Graphviz
			//system("dot -Tpng -o process_tree.png process_tree.dot");

			return SUCCESS;
		}

		////////////////////////////////////////
		
		
	
		//////////////////////


		/// This shows how to do exec with environ (but is not available on MacOs)
		// extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// TODO: do your own exec with path resolving using execv()
		// do so by replacing the execvp call below
		//execvp(command->name, command->args); // exec+args+path

		char *env_path = getenv("PATH");
		char *alias_command_tok = strtok(env_path, ":");
		char *path = calloc(200, sizeof(char));
		while(alias_command_tok != NULL){
			strcpy(path, alias_command_tok);
			strcat(path, "/");
			strcat(path, command->name);
			execv(path, command->args);
			printf("%s" ,path);
			free(path);
			path = calloc(200, sizeof(char));
			alias_command_tok = strtok(NULL, ":");
			
		}

		
		exit(0);
	} else {


		// TODO: implement background processes here

		if(!command->background){
			waitpid(pid, NULL, 0); // wait for child process to finish
		}
		return SUCCESS;
	}

	// TODO: your implementation here

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}