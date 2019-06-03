/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_module_history.h
 * Author: mingchen
 *
 * Created on March 25, 2016, 3:55 PM
 */

#ifndef SBASH_MODULE_HISTORY_H
#define SBASH_MODULE_HISTORY_H

void history_put(char* str);

char* history_get(int index);

void history_clean();

int history_getSize();

char* history_rSearch(int* startFrom, char* query);

int history_save(char* path);

int history_initAndLoad(char* path);

#endif /* SBASH_MODULE_HISTORY_H */

