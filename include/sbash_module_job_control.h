/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_module_job_control.h
 * Author: mingchen
 *
 * Created on March 13, 2016, 6:53 PM
 */

#ifndef SBASH_MODULE_JOB_CONTROL_H
#define SBASH_MODULE_JOB_CONTROL_H

#include "sbash_execution.h"

Job* JOB_LIST;
int JOB_LIST_SIZE;
int JOB_LIST_CAPACITY;

void init_JobControl();

Job* addJob(Job* job);

int removeJob(Job* job);

int removeJobIndex(int index);

void printAllJob();

void printAJob(int i);

Job* getJob(int index);

void updateAllJobStatus(int noPrint);

void cleanUp_jobControl();

#endif /* SBASH_MODULE_JOB_CONTROL_H */

