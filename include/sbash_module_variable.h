/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_module_variable.h
 * Author: shengchun
 *
 * Created on March 20, 2016, 6:21 PM
 */

#ifndef SBASH_MODULE_VARIABLE_H
#define SBASH_MODULE_VARIABLE_H
typedef struct variable_element{
    char* key;
    void* value;
} variable_element;

variable_element QuestionMark;

void variable_init();
void* variable_get(char* key);
void variable_set(char* key, void* value);

#endif /* SBASH_MODULE_VARIABLE_H */

