/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_320sh.h
 * Author: mingchen
 *
 * Created on March 14, 2016, 8:21 PM
 */

#ifndef SBASH_320SH_H
#define SBASH_320SH_H

#define HISTORY_LOG_PATH "history.log"

int init (int argc, char ** argv, char **envp);

void do_nothing_handler(int sig);

void cleanUp(int exitStatus);

#endif /* SBASH_320SH_H */

