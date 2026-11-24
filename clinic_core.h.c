#ifndef CLINIC_CORE_H
#define CLINIC_CORE_H

#define MAX_QUEUE 50
#define MAX_STACK 50
#define MAX_RECORDS 100
#define MAX_CATALOG 8

typedef struct {
    char department[25];
    char condition[45];
    char local_code[25];
    char icd11_code[25];
    char profile_tag[30];
    char medication[80];
    char dosage_timing[60];
    char recommended_diet[100];
    char restrictions[100];
} ClinicalProtocol;

typedef struct {
    int id;
    char name[45];
    char health_id[25];
    char department[25];
    char condition[45];
    char profile_tag[30];
} WaitingPatient;

typedef struct PatientRecord {
    int id;
    char name[45];
    char health_id[25];
    char department[25];
    char condition[45];
    char profile_tag[30];
    char local_code[25];
    char icd11_code[25];
    char medication[80];
    char dosage_timing[60];
    char recommended_diet[100];
    char restrictions[100];
    struct PatientRecord* next;
} PatientRecord;

#endif