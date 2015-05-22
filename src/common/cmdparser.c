/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * This file implements the Quake II command processor. Every command
 * which is send via the command line at startup, via the console and
 * via rcon is processed here and send to the apropriate subsystem.
 *
 * =======================================================================
 */

#include "header/common.h"

#define MAX_ALIAS_NAME 32
#define ALIAS_LOOP_COUNT 16

typedef struct cmd_function_s{
    struct cmd_function_s *next;
    char *name;
    xcommand_t function;
} cmd_function_t;

static cmd_function_t *cmd_functions; /* possible commands to execute */

typedef struct cmdalias_s{
    struct cmdalias_s *next;
    char name[MAX_ALIAS_NAME];
    char *value;
} cmdalias_t;

static char retval[256];
static int alias_count; /* for detecting runaway loops */
static cmdalias_t *cmd_alias;
static qboolean cmd_wait;
static int cmd_argc;
static int cmd_argc;
static char *cmd_argv[MAX_STRING_TOKENS];
static char *cmd_null_string = "";
static char cmd_args[MAX_STRING_CHARS];
static sizebuf_t cmd_text;
static byte cmd_text_buf[8192];
static char defer_text_buf[8192];

/*
 * Causes execution of the remainder of the command buffer to be delayed
 * until next frame.  This allows commands like: bind g "impulse 5 ;
 * +attack ; wait ; -attack ; impulse 2"
 */
static void Cmd_Wait_f(void){
    cmd_wait = true;
}

void Cbuf_Init(void){
    SZ_Init(&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
}

/*
 * Adds command text at the end of the buffer
 */
void Cbuf_AddText(const char *text){
    int l = strlen(text);
    if(cmd_text.cursize + l >= cmd_text.maxsize){
        Com_Printf("Cbuf_AddText: overflow\n");
        return;
    }
    SZ_Write(&cmd_text, text, strlen(text));
}

/*
 * Adds command text immediately after the current command
 * Adds a \n to the text
 */
void Cbuf_InsertText(const char *text){
    /* copy off any commands still remaining in the exec buffer */
    int templen = cmd_text.cursize;
    char *temp;
    if(templen){
        temp = Z_Malloc(templen);
        memcpy(temp, cmd_text.data, templen);
        SZ_Clear(&cmd_text);
    }else{
        temp = NULL;
    }
    /* add the entire text of the file */
    Cbuf_AddText(text);
    /* add the copied off data */
    if(templen){
        SZ_Write(&cmd_text, temp, templen);
        Z_Free(temp);
    }
}

void Cbuf_CopyToDefer(void){
    memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
    defer_text_buf[cmd_text.cursize] = 0;
    cmd_text.cursize = 0;
}

void Cbuf_InsertFromDefer(void){
    Cbuf_InsertText(defer_text_buf);
    defer_text_buf[0] = 0;
}

void Cbuf_ExecuteText(int exec_when, const char *text){
    switch (exec_when){
        case EXEC_NOW:
            Cmd_ExecuteString(text);
            break;
        case EXEC_INSERT:
            Cbuf_InsertText(text);
            break;
        case EXEC_APPEND:
            Cbuf_AddText(text);
            break;
        default:
            Com_Error(ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
    }
}

void Cbuf_Execute(void){
    char line[1024];
    alias_count = 0; /* don't allow infinite alias loops */
    while(cmd_text.cursize){
        /* find a \n or ; line break */
        char* text = (char *)cmd_text.data;
        int quotes = 0;
        int i;
        for(i = 0; i < cmd_text.cursize; i++){
            if(text[i] == '"'){
                quotes++;
            }
            if(!(quotes & 1) && (text[i] == ';')){
                break; /* don't break if inside a quoted string */
            }
            if(text[i] == '\n'){
                break;
            }
        }
        if(i > sizeof(line) - 1){
            i = sizeof(line) - 1;
        }
        memcpy(line, text, i);
        line[i] = 0;
        /* delete the text from the command buffer and move remaining
           commands down this is necessary because commands (exec,
           alias) can insert data at the beginning of the text buffer */
        if(i == cmd_text.cursize){
            cmd_text.cursize = 0;
        }else{
            i++;
            cmd_text.cursize -= i;
            memmove(text, text + i, cmd_text.cursize);
        }
        /* execute the command line */
        Cmd_ExecuteString(line);
        if(cmd_wait){
            /* skip out while text still remains in buffer,
               leaving it for next frame */
            cmd_wait = false;
            break;
        }
    }
}

/*
 * Adds command line parameters as script statements Commands lead with
 * a +, and continue until another +
 *
 * Set commands are added early, so they are guaranteed to be set before
 * the client and server initialize for the first time.
 *
 * Other commands are added late, after all initialization is complete.
 */
void Cbuf_AddEarlyCommands(qboolean clear){
    for(int i = 0; i < COM_Argc(); i++){
        const char* s = COM_Argv(i);
        if(strcmp(s, "+set")){
            continue;
        }
        Cbuf_AddText(va("set %s %s\n", COM_Argv(i + 1), COM_Argv(i + 2)));
        if(clear){
            COM_ClearArgv(i);
            COM_ClearArgv(i + 1);
            COM_ClearArgv(i + 2);
        }
        i += 2;
    }
}

/*
 * Adds command line parameters as script statements
 * Commands lead with a + and continue until another + or -
 * quake +developer 1 +map amlev1
 *
 * Returns true if any late commands were added, which
 * will keep the demoloop from immediately starting
 */
qboolean Cbuf_AddLateCommands(void){
    /* build the combined string to parse from */
    int s = 0;
    int argc = COM_Argc();
    for(int i = 1; i < argc; i++){
        s += strlen(COM_Argv(i)) + 1;
    }
    if(s == 0){
        return false;
    }
    char* text = Z_Malloc(s + 1);
    text[0] = 0;
    for(int i = 1; i < argc; i++){
        strcat(text, COM_Argv(i));
        if(i != argc - 1){
            strcat(text, " ");
        }
    }
    /* pull out the commands */
    char* build = Z_Malloc(s + 1);
    build[0] = 0;
    for(int i = 0; i < s - 1; i++){
        if(text[i] == '+'){
            i++;
            int j;
            for(j = i; (text[j] != '+') && (text[j] != '-') && (text[j] != 0); j++) {
            }
            char c = text[j];
            text[j] = 0;
            strcat(build, text + i);
            strcat(build, "\n");
            text[j] = c;
            i = j - 1;
        }
    }
    qboolean ret = (build[0] != 0);
    if(ret){
        Cbuf_AddText(build);
    }
    Z_Free(text);
    Z_Free(build);
    return ret;
}

void Cmd_Exec_f(void){
    if(Cmd_Argc() != 2){
        Com_Printf("exec <filename> : execute a script file\n");
        return;
    }
    char* f;
    int len = FS_LoadFile(Cmd_Argv(1), (void **)&f);
    if(!f){
        Com_Printf("couldn't exec %s\n", Cmd_Argv(1));
        return;
    }
    Com_Printf("execing %s\n", Cmd_Argv(1));
    /* the file doesn't have a trailing 0, so we need to copy it off */
    char* f2 = Z_Malloc(len + 1);
    memcpy(f2, f, len);
    f2[len] = 0;
    Cbuf_InsertText(f2);
    Z_Free(f2);
    FS_FreeFile(f);
}

/*
 * Just prints the rest of the line to the console
 */
void Cmd_Echo_f(void){
    for(int i = 1; i < Cmd_Argc(); i++){
        Com_Printf("%s ", Cmd_Argv(i));
    }
    Com_Printf("\n");
}

/*
 * Creates a new command that executes
 * a command string (possibly ; seperated)
 */
void Cmd_Alias_f(void){
    if(Cmd_Argc() == 1){
        Com_Printf("Current alias commands:\n");
        for(cmdalias_t* a = cmd_alias; a; a = a->next){
            Com_Printf("%s : %s\n", a->name, a->value);
        }
        return;
    }
    const char* s = Cmd_Argv(1);
    if(strlen(s) >= MAX_ALIAS_NAME){
        Com_Printf("Alias name is too long\n");
        return;
    }
    /* if the alias already exists, reuse it */
    cmdalias_t *a;
    for(a = cmd_alias; a; a = a->next){
        if(!strcmp(s, a->name)){
            Z_Free(a->value);
            break;
        }
    }
    if(!a){
        a = Z_Malloc(sizeof(cmdalias_t));
        a->next = cmd_alias;
        cmd_alias = a;
    }
    strcpy(a->name, s);
    /* copy the rest of the command line */
    char cmd[1024];
    cmd[0] = 0; /* start out with a null string */
    int c = Cmd_Argc();
    for(int i = 2; i < c; i++){
        strcat(cmd, Cmd_Argv(i));
        if(i != (c - 1)){
            strcat(cmd, " ");
        }
    }
    strcat(cmd, "\n");
    a->value = CopyString(cmd);
}

int Cmd_Argc(void){
    return cmd_argc;
}

const char* Cmd_Argv(int arg){
    if((unsigned)arg >= cmd_argc){
        return cmd_null_string;
    }
    return cmd_argv[arg];
}

/*
 * Returns a single string containing argv(1) to argv(argc()-1)
 */
char* Cmd_Args(void){
    return cmd_args;
}

const char* Cmd_MacroExpandString(const char *text){
    static char expanded[MAX_STRING_CHARS];
    char temporary[MAX_STRING_CHARS];
    const char* scan = text;
    int len = strlen(scan);
    if (len >= MAX_STRING_CHARS){
        Com_Printf("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
        return NULL;
    }
    int count = 0;
    qboolean inquote = false;
    for(int i = 0; i < len; i++){
        if (scan[i] == '"'){
            inquote ^= 1;
        }
        if(inquote){
            continue; /* don't expand inside quotes */
        }
        if(scan[i] != '$'){
            continue;
        }
        /* scan out the complete macro */
        const char* start = scan + i + 1;
        const char* token = COM_Parse(&start);
        if(!start){
            continue;
        }
        token = Cvar_VariableString(token);
        int j = strlen(token);
        len += j;
        if(len >= MAX_STRING_CHARS){
            Com_Printf("Expanded line exceeded %i chars, discarded.\n",
                    MAX_STRING_CHARS);
            return NULL;
        }
        memcpy(temporary, scan, i);
        memcpy(temporary + i, token, j);
        strcpy(temporary + i + j, start);
        strcpy(expanded, temporary);
        scan = expanded;
        i--;
        if(++count == 100){
            Com_Printf("Macro expansion loop, discarded.\n");
            return NULL;
        }
    }
    if(inquote){
        Com_Printf("Line has unmatched quote, discarded.\n");
        return NULL;
    }

    return scan;
}

/*
 * Parses the given string into command line tokens.
 * $Cvars will be expanded unless they are in a quoted token
 */
void Cmd_TokenizeString(const char *text, qboolean macroExpand){
    /* clear the args from the last string */
    int i;
    for(i = 0; i < cmd_argc; i++){
        Z_Free(cmd_argv[i]);
    }
    cmd_argc = 0;
    cmd_args[0] = 0;
    /* macro expand the text */
    if(macroExpand){
        text = Cmd_MacroExpandString(text);
    }
    if(!text){
        return;
    }
    while(1){
        /* skip whitespace up to a /n */
        while(*text && *text <= ' ' && *text != '\n'){
            text++;
        }
        if(*text == '\n'){
            /* a newline seperates commands in the buffer */
            text++;
            break;
        }
        if(!*text){
            return;
        }
        /* set cmd_args to everything after the first arg */
        if(cmd_argc == 1){
            strcpy(cmd_args, text);
            /* strip off any trailing whitespace */
            int l = strlen(cmd_args) - 1;
            for( ; l >= 0; l--){
                if(cmd_args[l] <= ' '){
                    cmd_args[l] = 0;
                }else{
                    break;
                }
            }
        }
        const char* com_token = COM_Parse(&text);
        if(!text){
            return;
        }
        if(cmd_argc < MAX_STRING_TOKENS){
            cmd_argv[cmd_argc] = Z_Malloc(strlen(com_token) + 1);
            strcpy(cmd_argv[cmd_argc], com_token);
            cmd_argc++;
        }
    }
}

static const cmd_function_t* Cmd_CommandNamed(const char* name){
    for(cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next){
        if(!strcmp(name, cmd->name)){
            return cmd;
        }
    }
    return NULL;
}

void Cmd_AddCommand(char *cmd_name, xcommand_t function){
    /* fail if the command is a variable name */
    if(Cvar_VariableString(cmd_name)[0]){
        Cmd_RemoveCommand(cmd_name);
    }
    /* fail if the command already exists */
    if(Cmd_CommandNamed(cmd_name) != NULL){
        Com_Printf("Cmd_AddCommand: %s already defined\n", cmd_name);
        return;
    }
    cmd_function_t* cmd = Z_Malloc(sizeof(cmd_function_t));
    cmd->name = cmd_name;
    cmd->function = function;
    /* link the command in */
    cmd_function_t** pos = &cmd_functions;
    while(*pos && strcmp((*pos)->name, cmd->name) < 0){
        pos = &(*pos)->next;
    }
    cmd->next = *pos;
    *pos = cmd;
}

void Cmd_RemoveCommand(char *cmd_name){
    cmd_function_t** back = &cmd_functions;
    while(1){
        cmd_function_t* cmd = *back;
        if(!cmd){
            Com_Printf("Cmd_RemoveCommand: %s not added\n", cmd_name);
            return;
        }
        if(!strcmp(cmd_name, cmd->name)){
            *back = cmd->next;
            Z_Free(cmd);
            return;
        }
        back = &cmd->next;
    }
}

int qsort_strcomp(const void *s1, const void *s2){
    return strcmp(*(char **)s1, *(char **)s2);
}

static const cmdalias_t* Cmd_AliasNamed(const char* name){
    for(cmdalias_t* alias = cmd_alias; alias; alias = alias->next){
        if(!strcmp(name, alias->name)){
            return alias;
        }
    }
    return NULL;
}

const char* Cmd_CompleteCommand(const char *partial){
    if(partial == NULL || strlen(partial) == 0){
        return NULL;
    }
    const char *pmatch[1024];
    /* check for exact match */
    const cmd_function_t* command = Cmd_CommandNamed(partial);
    if(command != NULL){
        return command->name;
    }

    const cmdalias_t *alias = Cmd_AliasNamed(partial);
    if(alias != NULL){
        return alias->name;
    }

    const cvar_t *cvar = Cvar_VarNamed(partial);
    if(cvar != NULL){
        return cvar->name;
    }
    memset(pmatch, 0, sizeof(const char*));
    int i = 0;
    /* check for partial match */
    int len = strlen(partial);
    for(cmd_function_t* cmd = cmd_functions; cmd; cmd = cmd->next){
        if(!strncmp(partial, cmd->name, len)){
            pmatch[i] = cmd->name;
            i++;
        }
    }
    for(alias = cmd_alias; alias; alias = alias->next){
        if(!strncmp(partial, alias->name, len)){
            pmatch[i] = alias->name;
            i++;
        }
    }
    for(cvar = cvar_vars; cvar; cvar = cvar->next){
        if(!strncmp(partial, cvar->name, len)){
            pmatch[i] = cvar->name;
            i++;
        }
    }
    if(i){
        if(i == 1){
            return pmatch[0];
        }
        /* Sort it */
        qsort(pmatch, i, sizeof(pmatch[0]), qsort_strcomp);
        Com_Printf("\n\n", partial);
        for(int o = 0; o < i; o++){
            Com_Printf("  %s\n", pmatch[o]);
        }
        strcpy(retval, "");
        int p = 0;
        qboolean diff = false;
        while(!diff && p < 256){
            retval[p] = pmatch[0][p];
            for(int o = 0; o < i; o++){
                if(p > strlen(pmatch[o])){
                    continue;
                }
                if(retval[p] != pmatch[o][p]){
                    retval[p] = 0;
                    diff = true;
                }
            }
            p++;
        }
        return retval;
    }
    return NULL;
}

qboolean Cmd_IsComplete(const char *command){
    /* check for exact match */
    if(Cmd_CommandNamed(command) != NULL){
        return true;
    }
    if(Cmd_AliasNamed(command) != NULL){
        return true;
    }
    if(Cvar_VarNamed(command) != NULL){
        return true;
    }
    return false;
}

/*
 * A complete command line has been parsed, so try to execute it
 */
void Cmd_ExecuteString(const char *text){
    Cmd_TokenizeString(text, true);
    /* execute the command line */
    if(!Cmd_Argc()){
        return; /* no tokens */
    }
    /* check functions */
    const cmd_function_t* command = Cmd_CommandNamed(cmd_argv[0]);
    if(command != NULL){
        if(command->function != NULL){
            command->function();
        }else{
            Cmd_ExecuteString(va("cmd %s", text));
        }
        return;
    }
    /* check alias */
    const cmdalias_t* alias = Cmd_AliasNamed(cmd_argv[0]);
    if(alias != NULL){
        if(++alias_count == ALIAS_LOOP_COUNT){
            Com_Printf("ALIAS_LOOP_COUNT\n");
            return;
        }
        Cbuf_InsertText(alias->value);
        return;
    }
    /* check cvars */
    if(Cvar_Command(Cmd_Argv(0), Cmd_Argv(1), Cmd_Argc())){
        return;
    }
#ifndef DEDICATED_ONLY
    /* send it as a server command if we are connected */
    Cmd_ForwardToServer();
#endif
}

void Cmd_List_f(void){
    cmd_function_t *cmd;
    int i = 0;
    for(cmd = cmd_functions; cmd; cmd = cmd->next, i++){
        Com_Printf("%s\n", cmd->name);
    }
    Com_Printf("%i commands\n", i);
}

void Cmd_Init(void){
    /* register our commands */
    Cmd_AddCommand("cmdlist", Cmd_List_f);
    Cmd_AddCommand("exec", Cmd_Exec_f);
    Cmd_AddCommand("echo", Cmd_Echo_f);
    Cmd_AddCommand("alias", Cmd_Alias_f);
    Cmd_AddCommand("wait", Cmd_Wait_f);
}
