#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten/emscripten.h>
#include "clinic_core.h"

/* Clinical Protocol Reference Catalog */
static ClinicalProtocol catalog[MAX_CATALOG] = {
    {"General Medicine", "Acute Hyperacidity & Gastritis", "REF-GM-014", "TM2-DA01.1", "Type-P Profile",
     "Antacid Herbal Compound (3g) + Cooling Mineral Tabs", "Before meals with lukewarm water",
     "Barley, steamed rice, pomegranate, coconut water", "Spicy, sour, fermented foods, late meals"},
    {"Orthopedics", "Chronic Osteoarthritis", "REF-OR-089", "TM2-FA20.0", "Type-V Profile",
     "Joint Support Resin Tabs + Therapeutic Sesame Oil", "After meals with warm water",
     "Warm oil massage, garlic, drumstick, warm broth", "Cold exposure, dry foods, heavy exertion"},
    {"Pulmonology", "Bronchial Asthma & Wheezing", "REF-PU-042", "TM2-CA23.2", "Type-KV Profile",
     "Herbal Expectorant Powder + Respiratory Balm", "Twice daily with honey",
     "Warm water, horse gram soup, steam inhalation", "Curd, cold beverages, daytime sleep, dust"},
    {"Endocrinology", "Type-2 Metabolic Diabetes", "REF-EN-102", "TM2-5A11.0", "Type-K Profile",
     "Turmeric-Amla Extract + Metabolic Support Tabs", "30 minutes before meals",
     "Barley, bitter gourd, fenugreek, daily brisk walk", "Refined sugars, sedentary habits, heavy carbs"},
    {"Rheumatology", "Inflammatory Joint Pain", "REF-RH-031", "TM2-FA20.2", "Type-AV Profile",
     "Ashwagandha Compound + Analgesic Herbal Oil", "After food with warm milk",
     "Light warm meals, warm water fomentation", "Cold baths, sour pickles, starchy tubers"},
    {"Neurology", "Migraine & Tension Headache", "REF-NE-054", "TM2-8A80.0", "Type-PV Profile",
     "Neuro-Calming Tonic + Nasal Herbal Drops", "Morning and bedtime with water",
     "Almonds, ghee, soaked raisins, regular sleep", "Skipping meals, bright screen glare, caffeine"}
};

/* Active Waiting Line */
static WaitingPatient waiting_line[MAX_QUEUE];
static int front = -1, rear = -1;
static int next_patient_id = 1001;

/* Patient Electronic Health Records */
static PatientRecord* records_head = NULL;

/* Clinician Rollback History */
static int rollback_ids[MAX_STACK];
static int top = -1;

static char json_buffer[32768];

EMSCRIPTEN_KEEPALIVE
int register_patient(const char* name, const char* health_id, const char* department,
                     const char* condition, const char* profile_tag) {
    if (rear >= MAX_QUEUE - 1) return -1;
    if (front == -1) front = 0;
    rear++;
    waiting_line[rear].id = next_patient_id++;
    strncpy(waiting_line[rear].name, name, 44);
    waiting_line[rear].name[44] = '\0';
    strncpy(waiting_line[rear].health_id, health_id, 24);
    waiting_line[rear].health_id[24] = '\0';
    strncpy(waiting_line[rear].department, department, 24);
    waiting_line[rear].department[24] = '\0';
    strncpy(waiting_line[rear].condition, condition, 44);
    waiting_line[rear].condition[44] = '\0';
    strncpy(waiting_line[rear].profile_tag, profile_tag, 29);
    waiting_line[rear].profile_tag[29] = '\0';
    return waiting_line[rear].id;
}

EMSCRIPTEN_KEEPALIVE
int consult_next_patient() {
    if (front == -1 || front > rear) return -1;
    WaitingPatient p = waiting_line[front++];
    if (front > rear) front = rear = -1;

    int idx = 0;
    for (int i = 0; i < 6; i++) {
        if (strcmp(catalog[i].condition, p.condition) == 0) {
            idx = i;
            break;
        }
    }

    PatientRecord* rec = (PatientRecord*)malloc(sizeof(PatientRecord));
    if (!rec) return -1;

    rec->id = p.id;
    strcpy(rec->name, p.name);
    strcpy(rec->health_id, p.health_id);
    strcpy(rec->department, p.department);
    strcpy(rec->condition, p.condition);
    strcpy(rec->profile_tag, p.profile_tag);
    strcpy(rec->local_code, catalog[idx].local_code);
    strcpy(rec->icd11_code, catalog[idx].icd11_code);
    strcpy(rec->medication, catalog[idx].medication);
    strcpy(rec->dosage_timing, catalog[idx].dosage_timing);
    strcpy(rec->recommended_diet, catalog[idx].recommended_diet);
    strcpy(rec->restrictions, catalog[idx].restrictions);

    rec->next = records_head;
    records_head = rec;

    if (top < MAX_STACK - 1) {
        rollback_ids[++top] = rec->id;
    }
    return rec->id;
}

EMSCRIPTEN_KEEPALIVE
int undo_last_consultation() {
    if (top < 0) return -1;
    int target_id = rollback_ids[top--];

    PatientRecord *curr = records_head, *prev = NULL;
    while (curr != NULL && curr->id != target_id) {
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL) {
        if (prev == NULL) records_head = curr->next;
        else prev->next = curr->next;
        free(curr);
        return target_id;
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
const char* fetch_clinic_state_json() {
    strcpy(json_buffer, "{\"queue\":[");
    char entry[512];
    int first = 1;

    if (front != -1 && front <= rear) {
        for (int i = front; i <= rear; i++) {
            if (!first) strcat(json_buffer, ",");
            snprintf(entry, sizeof(entry),
                     "{\"id\":%d,\"name\":\"%s\",\"healthId\":\"%s\",\"dept\":\"%s\",\"condition\":\"%s\",\"profile\":\"%s\"}",
                     waiting_line[i].id, waiting_line[i].name, waiting_line[i].health_id,
                     waiting_line[i].department, waiting_line[i].condition, waiting_line[i].profile_tag);
            strcat(json_buffer, entry);
            first = 0;
        }
    }

    strcat(json_buffer, "],\"records\":[");
    PatientRecord* curr = records_head;
    first = 1;
    while (curr != NULL) {
        if (!first) strcat(json_buffer, ",");
        char item[1024];
        snprintf(item, sizeof(item),
                 "{\"id\":%d,\"name\":\"%s\",\"healthId\":\"%s\",\"dept\":\"%s\",\"condition\":\"%s\","
                 "\"profile\":\"%s\",\"localCode\":\"%s\",\"icd11\":\"%s\",\"medication\":\"%s\","
                 "\"timing\":\"%s\",\"diet\":\"%s\",\"restrictions\":\"%s\"}",
                 curr->id, curr->name, curr->health_id, curr->department, curr->condition,
                 curr->profile_tag, curr->local_code, curr->icd11_code, curr->medication,
                 curr->dosage_timing, curr->recommended_diet, curr->restrictions);
        strcat(json_buffer, item);
        first = 0;
        curr = curr->next;
    }

    strcat(json_buffer, "],\"canUndo\":");
    strcat(json_buffer, (top >= 0) ? "true}" : "false}");
    return json_buffer;
}