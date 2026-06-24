#include <3ds.h>
#include <citro2d.h>
#include "inpost_api.h"
#include "scene_manager.h"
#include "main.h"
#include "cwav_shit.h"
#include "sprites.h"
#include "text.h"
#include <math.h>
#include <stdio.h>
#include <jansson.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "request.h"
#include "cJSON.h"
#include "utils.h"
ResponseBuffer send_phone_numer = {NULL, 0, 0, false};
ResponseBuffer send_kod_sms = {NULL, 0, 0, false};
ResponseBuffer paczkas = {NULL, 0, 0, false};
ResponseBuffer sent_paczkas = {NULL, 0, 0, false};
ResponseBuffer returned_paczkas = {NULL, 0, 0, false};
ResponseBuffer get_refresh_token = {NULL, 0, 0, false};
ResponseBuffer dane_usera = {NULL, 0, 0, false};
ResponseBuffer balans_konta = {NULL, 0, 0, false};
ResponseBuffer validate_paczkomat = {NULL, 0, 0, false};
ResponseBuffer open_paczkomat = {NULL, 0, 0, false};
ResponseBuffer terminate_paczka = {NULL, 0, 0, false};
ResponseBuffer jak_tam_skrytka = {NULL, 0, 0, false};
ResponseBuffer paczkomat_image = {NULL, 0, 0, false};
ResponseBuffer testreq = {NULL, 0, 0, false};

char* refreshToken = NULL;
char* authToken = NULL;
int inpointsy = 0;

bool pers_data_done;


Paczka paczka_list[MAX_PACZKAS];
int paczka_count = 0;



void testrequest(){
    queue_request("https://inpost.pl/404", NULL, NULL, &testreq, false);
}

// wyślij prośbe o kod sms na podany numer
void sendNumerTel(char numertel[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[128]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"}}", numertel);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account", request, headers, &send_phone_numer, false);
}

// wyślij kod sms z wcześniej dostanego sms'a
void sendKodSMS(char numertel[60], char kodsms[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[256]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"},\"smsCode\":\"%s\",\"devicePlatform\":\"Android\"}", numertel, kodsms);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account/verification", request, headers, &send_kod_sms, false);
}

// odśwież token używając refresha (tego co dostawałeś przy logowaniu bądz po odświeżeniu)
void refresh_the_Token(char* refresh) {
    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    char request[256]; 
    snprintf(request, sizeof(request), "{\"refreshToken\":\"%s\"}", refresh);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/authenticate", request, headers, &get_refresh_token, false);

    curl_slist_free_all(headers);
}

void parseRefreshedToken(const char* json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        log_to_file("[parseRefreshedToken] parse failed");
        return;
    }

    cJSON *accesstoken  = cJSON_GetObjectItemCaseSensitive(root, "authToken");
    cJSON *refreshtoken = cJSON_GetObjectItemCaseSensitive(root, "refreshToken");

    if (cJSON_IsString(accesstoken) && accesstoken->valuestring) {
        free(authToken);
        authToken = strdup(accesstoken->valuestring);
    }

    if (cJSON_IsString(refreshtoken) && refreshtoken->valuestring) {
        free(refreshToken);
        refreshToken = strdup(refreshtoken->valuestring);
    }

    cJSON *rootenmach = NULL;
    open_json("/3ds/InPost3DS/in_post_dane.json", &rootenmach);

    if (!rootenmach) {
        log_to_file("[parseRefreshedToken] failed to open stored json");
        cJSON_Delete(root);
        return;
    }

    if (cJSON_IsString(accesstoken) && accesstoken->valuestring) {
        cJSON *newAccess = cJSON_CreateString(accesstoken->valuestring);
        if (cJSON_ReplaceItemInObject(rootenmach, "access", newAccess) == 0) {
            cJSON_AddItemToObject(rootenmach, "access", newAccess);
        }
    }

    if (cJSON_IsString(refreshtoken) && refreshtoken->valuestring) {
        cJSON *newRefresh = cJSON_CreateString(refreshtoken->valuestring);
        if (cJSON_ReplaceItemInObject(rootenmach, "refresh", newRefresh) == 0) {
            cJSON_AddItemToObject(rootenmach, "refresh", newRefresh);
        }
    }

    save_json("/3ds/InPost3DS/in_post_dane.json", rootenmach);

    cJSON_Delete(rootenmach);
    cJSON_Delete(root);
}

// pobierz paczki
void getPaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas, false);
}

// (używane w dev buildach) parsuj test dane
void parseFakePaczkas() {
    bool ignore_bo_kurier = false;

    cJSON *root = NULL;
    open_json("romfs:/test_data.json", &root);

    cJSON *parcels = cJSON_GetObjectItemCaseSensitive(root, "parcels");
    if (!cJSON_IsArray(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        cJSON_Delete(root);
        return;
    }

    paczka_count = 0;

    size_t total_parcels = cJSON_GetArraySize(parcels);

    for (size_t i = total_parcels; i > 0; i--) {
        

        cJSON *parcel = cJSON_GetArrayItem(parcels, i - 1);
    


        if (paczka_count >= MAX_PACZKAS)
            break;

        Paczka *p = &paczka_list[paczka_count];
        
        if (strstr(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(parcel, "shipmentType")), "courier") != NULL) {
            p->courier_paczka = true;
            ignore_bo_kurier = true;
        } else {
            p->courier_paczka = false;
            ignore_bo_kurier = false;
        }
       
        jstrcpy(p->shipmentNumber, sizeof(p->shipmentNumber),
                cJSON_GetObjectItemCaseSensitive(parcel, "shipmentNumber"));
        jstrcpy(p->statusGroup, sizeof(p->statusGroup),
                cJSON_GetObjectItemCaseSensitive(parcel, "statusGroup"));
        jstrcpy(p->parcelSize, sizeof(p->parcelSize),
                cJSON_GetObjectItemCaseSensitive(parcel, "parcelSize"));
        if (!ignore_bo_kurier) {
            jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                    cJSON_GetObjectItemCaseSensitive(parcel, "pickUpDate"));     
            jstrcpy(p->opencode, sizeof(p->opencode),
                    cJSON_GetObjectItemCaseSensitive(parcel, "openCode"));
            if (cJSON_GetObjectItemCaseSensitive(parcel, "openCode")) {
                p->paczka_openable = true;
            } else {
                p->paczka_openable = false;
            }
            jstrcpy(p->qrCode, sizeof(p->qrCode),
                    cJSON_GetObjectItemCaseSensitive(parcel, "qrCode"));
        } 
        jstrcpy(p->status, sizeof(p->status),
                cJSON_GetObjectItemCaseSensitive(parcel, "status"));
        if (strstr(p->status, "DELIVERED")) {
            p->paczka_openable = false;
        } 
        if (ignore_bo_kurier) {
            cJSON *sender = cJSON_GetObjectItemCaseSensitive(parcel, "sender");
            cJSON *sender_name = cJSON_GetObjectItemCaseSensitive(sender, "name");
            jstrcpy(p->pickupPointName, sizeof(p->pickupPointName), sender_name);
        }
        
        if (!ignore_bo_kurier) {
            cJSON *pickup = cJSON_GetObjectItemCaseSensitive(parcel, "pickUpPoint");
            if (cJSON_IsObject(pickup)) {
                jstrcpy(p->pickupPointName, sizeof(p->pickupPointName),
                        cJSON_GetObjectItemCaseSensitive(pickup, "name"));
                jstrcpy(p->imageUrl, sizeof(p->imageUrl),
                        cJSON_GetObjectItemCaseSensitive(pickup, "imageUrl"));

                cJSON *addr = cJSON_GetObjectItemCaseSensitive(pickup, "addressDetails");
                if (cJSON_IsObject(addr)) {
                    jstrcpy(p->city, sizeof(p->city),
                            cJSON_GetObjectItemCaseSensitive(addr, "city"))   ;
                    jstrcpy(p->street, sizeof(p->street),
                            cJSON_GetObjectItemCaseSensitive(addr, "street"));
                }
                cJSON *location = cJSON_GetObjectItemCaseSensitive(pickup, "location");
                if (cJSON_IsObject(location)) {
                    cJSON *lat = cJSON_GetObjectItemCaseSensitive(location, "latitude");
                    cJSON *lon = cJSON_GetObjectItemCaseSensitive(location, "longitude");

                    if (cJSON_IsNumber(lat))
                        p->latitude = (float)cJSON_GetNumberValue(lat);
                    if (cJSON_IsNumber(lon))
                        p->longitude = (float)cJSON_GetNumberValue(lon);
                }
            }
        }
        cJSON *receiver = cJSON_GetObjectItemCaseSensitive(parcel, "receiver");
        cJSON *receiver_phone = cJSON_GetObjectItemCaseSensitive(receiver, "phoneNumber");
        if (cJSON_IsObject(receiver_phone)) {
            jstrcpy(p->phonePrefix, sizeof(p->phonePrefix),
                    cJSON_GetObjectItemCaseSensitive(receiver_phone, "prefix"));
            jstrcpy(p->phoneNumber, sizeof(p->phoneNumber),
                    cJSON_GetObjectItemCaseSensitive(receiver_phone, "value"));
        }

        
        cJSON *eventsArray = cJSON_GetObjectItemCaseSensitive(parcel, "events");
        if (cJSON_IsArray(eventsArray) && cJSON_GetArraySize(eventsArray) > 0) {
            cJSON *latest = cJSON_GetArrayItem(eventsArray, 0); 
            jstrcpy(p->latestEvent, sizeof(p->latestEvent), cJSON_GetObjectItemCaseSensitive(latest, "eventTitle"));
        } else {
            p->latestEvent[0] = '\0';
        }
        

        
        p->eventCount = 0;
        cJSON *eventLog = cJSON_GetObjectItemCaseSensitive(parcel, "events");

        jstrcpy(p->storedDate, sizeof(p->storedDate),
                cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(eventLog, 0), "date"));
        if (cJSON_IsArray(eventLog)) {
            cJSON *event;

            cJSON_ArrayForEach(event, eventLog) {
                if (p->eventCount >= MAX_EVENTS)
                    break;

                PaczkaEvent *ev = &p->events[p->eventCount];

                jstrcpy(ev->name, sizeof(ev->name),
                        cJSON_GetObjectItemCaseSensitive(event, "eventTitle"));
                jstrcpy(ev->date, sizeof(ev->date),
                        cJSON_GetObjectItemCaseSensitive(event, "date"));

                p->eventCount++;
            }
        }

        paczka_count++;
    }

    log_to_file("[parsePaczkas] Parsed %d paczkas", paczka_count);

    cJSON_Delete(root);
}

// parsuj paczki do structa "paczka_list"
void parsePaczkas(const char* json) {
    bool ignore_bo_kurier = false;

    cJSON *root = cJSON_Parse(json);
    

    cJSON *parcels = cJSON_GetObjectItemCaseSensitive(root, "parcels");
    if (!cJSON_IsArray(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        cJSON_Delete(root);
        return;
    }

    paczka_count = 0;

    size_t total_parcels = cJSON_GetArraySize(parcels);
    
    for (size_t i = total_parcels; i > 0; i--) {
        cJSON *parcel = cJSON_GetArrayItem(parcels, i - 1);
    

        if (paczka_count >= MAX_PACZKAS)
            break;

        Paczka *p = &paczka_list[paczka_count];
        
        if (strstr(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(parcel, "shipmentType")), "courier") != NULL) {
            p->courier_paczka = true;
            ignore_bo_kurier = true;
        } else {
            p->courier_paczka = false;
            ignore_bo_kurier = false;
        }
       
        jstrcpy(p->shipmentNumber, sizeof(p->shipmentNumber),
                cJSON_GetObjectItemCaseSensitive(parcel, "shipmentNumber"));
        jstrcpy(p->statusGroup, sizeof(p->statusGroup),
                cJSON_GetObjectItemCaseSensitive(parcel, "statusGroup"));
        jstrcpy(p->parcelSize, sizeof(p->parcelSize),
                cJSON_GetObjectItemCaseSensitive(parcel, "parcelSize"));
        if (!ignore_bo_kurier) {
            jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                    cJSON_GetObjectItemCaseSensitive(parcel, "pickUpDate"));     
            jstrcpy(p->opencode, sizeof(p->opencode),
                    cJSON_GetObjectItemCaseSensitive(parcel, "openCode"));
            if (cJSON_GetObjectItemCaseSensitive(parcel, "openCode")) {
                p->paczka_openable = true;
            } else {
                p->paczka_openable = false;
            }
            jstrcpy(p->qrCode, sizeof(p->qrCode),
                    cJSON_GetObjectItemCaseSensitive(parcel, "qrCode"));
        } 
        jstrcpy(p->status, sizeof(p->status),
                cJSON_GetObjectItemCaseSensitive(parcel, "status"));
        if (strstr(p->status, "DELIVERED")) {
            p->paczka_openable = false;
        } 
        if (ignore_bo_kurier) {
            cJSON *sender = cJSON_GetObjectItemCaseSensitive(parcel, "sender");
            cJSON *sender_name = cJSON_GetObjectItemCaseSensitive(sender, "name");
            jstrcpy(p->pickupPointName, sizeof(p->pickupPointName), sender_name);
        }
        
        if (!ignore_bo_kurier) {
            cJSON *pickup = cJSON_GetObjectItemCaseSensitive(parcel, "pickUpPoint");
            if (cJSON_IsObject(pickup)) {
                jstrcpy(p->pickupPointName, sizeof(p->pickupPointName),
                        cJSON_GetObjectItemCaseSensitive(pickup, "name"));
                jstrcpy(p->imageUrl, sizeof(p->imageUrl),
                        cJSON_GetObjectItemCaseSensitive(pickup, "imageUrl"));

                cJSON *addr = cJSON_GetObjectItemCaseSensitive(pickup, "addressDetails");
                if (cJSON_IsObject(addr)) {
                    jstrcpy(p->city, sizeof(p->city),
                            cJSON_GetObjectItemCaseSensitive(addr, "city"));
                    jstrcpy(p->street, sizeof(p->street),
                            cJSON_GetObjectItemCaseSensitive(addr, "street"));
                }
                cJSON *location = cJSON_GetObjectItemCaseSensitive(pickup, "location");
                if (cJSON_IsObject(location)) {
                    cJSON *lat = cJSON_GetObjectItemCaseSensitive(location, "latitude");
                    cJSON *lon = cJSON_GetObjectItemCaseSensitive(location, "longitude");

                    if (cJSON_IsNumber(lat))
                        p->latitude = (float)cJSON_GetNumberValue(lat);
                    if (cJSON_IsNumber(lon))
                        p->longitude = (float)cJSON_GetNumberValue(lon);
                }
            }
        }
        cJSON *receiver = cJSON_GetObjectItemCaseSensitive(parcel, "receiver");
        cJSON *receiver_phone = cJSON_GetObjectItemCaseSensitive(receiver, "phoneNumber");
        if (cJSON_IsObject(receiver_phone)) {
            jstrcpy(p->phonePrefix, sizeof(p->phonePrefix),
                    cJSON_GetObjectItemCaseSensitive(receiver_phone, "prefix"));
            jstrcpy(p->phoneNumber, sizeof(p->phoneNumber),
                    cJSON_GetObjectItemCaseSensitive(receiver_phone, "value"));
        }

        
        cJSON *eventsArray = cJSON_GetObjectItemCaseSensitive(parcel, "events");
        if (cJSON_IsArray(eventsArray) && cJSON_GetArraySize(eventsArray) > 0) {
            cJSON *latest = cJSON_GetArrayItem(eventsArray, 0); 
            jstrcpy(p->latestEvent, sizeof(p->latestEvent), cJSON_GetObjectItemCaseSensitive(latest, "eventTitle"));
        } else {
            p->latestEvent[0] = '\0';
        }
        

        
        p->eventCount = 0;
        cJSON *eventLog = cJSON_GetObjectItemCaseSensitive(parcel, "events");

        cJSON *firstEvent = cJSON_GetArrayItem(eventLog, 0);
        if (cJSON_IsObject(firstEvent)) {
            jstrcpy(p->storedDate, sizeof(p->storedDate),
                    cJSON_GetObjectItemCaseSensitive(firstEvent, "date"));
        } else {
            p->storedDate[0] = '\0';
        }
        if (cJSON_IsArray(eventLog)) {
            cJSON *event;

            cJSON_ArrayForEach(event, eventLog) {
                if (p->eventCount >= MAX_EVENTS)
                    break;

                PaczkaEvent *ev = &p->events[p->eventCount];

                jstrcpy(ev->name, sizeof(ev->name),
                        cJSON_GetObjectItemCaseSensitive(event, "eventTitle"));
                jstrcpy(ev->date, sizeof(ev->date),
                        cJSON_GetObjectItemCaseSensitive(event, "date"));

                p->eventCount++;
            }
        }

        paczka_count++;
    }

    log_to_file("[parsePaczkas] Parsed %d paczkas", paczka_count);

    cJSON_Delete(root);
}

void getWyslanePaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas, false);

}

// pobierz zwrócone paczki
void getReturnedPaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas, false);

}

// pobierz dane jak imie i nazwisko, ulubiony paczkomat itp
void getPersonalData() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera, false);
}

// przeparsuj dane z getPersonalData do personal_dane.json
void parsePersonalData(const char* json){
    pers_data_done = false;
    cJSON *root = cJSON_Parse(json);
    cJSON *rootenmach = cJSON_CreateObject();

    
    cJSON *personal = cJSON_GetObjectItemCaseSensitive(root, "personal");
    cJSON *firstname = cJSON_GetObjectItemCaseSensitive(personal, "firstName");
    cJSON *lastname = cJSON_GetObjectItemCaseSensitive(personal, "lastName");
    cJSON *phone = cJSON_GetObjectItemCaseSensitive(personal, "phoneNumber");
    cJSON *phoneprefix = cJSON_GetObjectItemCaseSensitive(personal, "phoneNumberPrefix");
    cJSON *email = cJSON_GetObjectItemCaseSensitive(personal, "email");

    
    cJSON *delivery = cJSON_GetObjectItemCaseSensitive(root, "delivery");
    cJSON *points = cJSON_GetObjectItemCaseSensitive(delivery, "points");
    cJSON *items = cJSON_GetObjectItemCaseSensitive(points, "items");
    cJSON *pierwszy_paczkomat = cJSON_GetArrayItem(items, 0);
    cJSON *id_paczkomatu = cJSON_GetObjectItemCaseSensitive(pierwszy_paczkomat, "name");
    cJSON *dane_adresowe = cJSON_GetObjectItemCaseSensitive(pierwszy_paczkomat, "addressLines");

    

    cJSON_AddItemToObject(rootenmach, "firstname", cJSON_CreateString(cJSON_GetStringValue(firstname)));
    cJSON_AddItemToObject(rootenmach, "lastname", cJSON_CreateString(cJSON_GetStringValue(lastname)));
    cJSON_AddItemToObject(rootenmach, "phone_number", cJSON_CreateString(cJSON_GetStringValue(phone)));
    cJSON_AddItemToObject(rootenmach, "phoneprefix", cJSON_CreateString(cJSON_GetStringValue(phoneprefix)));
    cJSON_AddItemToObject(rootenmach, "email", cJSON_CreateString(cJSON_GetStringValue(email)));
    cJSON_AddItemToObject(rootenmach, "id_pref_paczkomatu", cJSON_CreateString(cJSON_GetStringValue(id_paczkomatu)));
    cJSON *linia;

    cJSON *adres_array = cJSON_CreateArray();
    cJSON_ArrayForEach(linia, dane_adresowe) {
        cJSON_AddItemToArray(adres_array, cJSON_CreateString(cJSON_GetStringValue(linia)));
    }
    cJSON_AddItemToObject(rootenmach, "adres_pref_paczkomatu", adres_array);


    save_json("/3ds/InPost3DS/personal_dane.json", rootenmach);
    pers_data_done = true;
    cJSON_Delete(root);
}

// pobierz inpointsy (never used lol)
void getInPointsBalance() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/loyalty/v1/wallet/balance", NULL, headers, &balans_konta, false);
    free(authheader);
}

// parsuj inpointsy (never used lol)
void parseInPointsBalance(const char* json){
    cJSON *root = cJSON_Parse(json);
    cJSON *balans = cJSON_GetObjectItemCaseSensitive(root, "currentBalance");
    inpointsy = cJSON_GetNumberValue(balans);
    cJSON_Delete(root);
}

// pobierz status paczkomatu który otwierasz
void getPaczkomatStatus(const char* shipmentNumber, const char* openCode, const char* recieverPhoneNumber, const char* recieverPhonePrefix, float latitude, float longitude) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    cJSON *data = cJSON_CreateObject();

    cJSON *parcel = cJSON_CreateObject();
    cJSON_AddItemToObject(parcel, "shipmentNumber", cJSON_CreateString(shipmentNumber));
    cJSON_AddItemToObject(parcel, "openCode", cJSON_CreateString(openCode));

    cJSON *phone = cJSON_CreateObject();
    cJSON_AddItemToObject(phone, "prefix", cJSON_CreateString(recieverPhonePrefix));
    cJSON_AddItemToObject(phone, "value", cJSON_CreateString(recieverPhoneNumber));

    cJSON_AddItemToObject(parcel, "recieverPhoneNumber", phone);

    cJSON *geo = cJSON_CreateObject();
    cJSON_AddItemToObject(geo, "latitude", cJSON_CreateNumber(latitude));
    cJSON_AddItemToObject(geo, "longitude", cJSON_CreateNumber(longitude));
    cJSON_AddItemToObject(geo, "accuracy", cJSON_CreateNumber(13.365));

    cJSON_AddItemToObject(data, "parcel", parcel);
    cJSON_AddItemToObject(data, "geoPoint", geo);
    char *data_json = cJSON_Print(data);
    free(authheader);
    queue_request("https://api-inmobile-pl.easypack24.net/v2/collect/validate", data_json, headers, &validate_paczkomat, false);
    cJSON_Delete(data);
    free(data_json);
}

// sezamie otwórz się
void openPaczkomat(const char* uuid) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/open", data_json, headers, &open_paczkomat, false);
    cJSON_Delete(data);
    free(data_json);
}

// .........:!!^..:77~:......^??:...........:.:~~:.....::::^^^~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!77777~.....::::..:!!!!!!!!!!~~~~~~~~~~~~~~~~~!!!!77777777?????7!:...
// ...............:7JJ7!......^:...........^~^........:::::^^^~~~~~~~~~!!!!!!!!7??7!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!77777!!!!777777777!......::^:...^!!!!!!!!~~~~~~~~~~~~~~~~~!!!7777777?????????7:...
// .................:!Y5~..................:::.....:::.::::^^^~~~~~~~!!77!!!!7775P57!!!!!!!!77!!!!!!!!!!!!!!!!!!!!!!!!!!!7!!!!!!!!777777777!....^^:^~!:..:!!!!!!!!~~~~~~~~~~~~~~~~!!!77777777??????????^...
// ...................^~.........................::7J7:::::^^^^~~~~~!!?YYJ777777J5Y7!77!!!!7?J7!!!!!!!7!!!!!!!!!!!!!!!!!!!!!!!!!77777777777!...:77!7?!....!!!!!!!!!~~~~~~~~~~~~~~!!!!77777777?????????7^...
// .....................^^:.....................:::^~^::::::^^^^^~~~~!7Y5Y7777777777777!!!!7777!!!77777!!~~~~~~~~~~~!!!!!!!!!!!!!7777777777!:...!?7777:...~!!!!!!!!~~~~~~~~~~~~~!!!!777777777?????????~....
// .........:::^:......:YP?.......................::~~~~!^::::^^^^~~~!!77!~~!7777777777777777!!!!!7777!!~~~~~~~~~~~~~~!!!!!!~~!!!!777777777?^...^^..:~...:7!!!!!!!!~~~~~~~~~~~!!!!!!777777777?????????^....
// :::^^^^^~77777~~~~~^^?J7^:....................::^~~~~^^::::^^^^~~~!7~....:^!7777777777???7!!!!77!!!!!~~~~~~~~~~~~~~~~~~~~~~~~~!!!7777777?^:..:........^!!!!!!!~~~~~~~~~~~~!!!!!!!!77777777????????7:....
// ~!!!77777????????7!!~~~~^^::.............:~~:::^^~!!^:::^^^:^7??!!77:......:^!777777777777!!!!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!!77777777^::.......:~!!!!!~~~~~~~~~~~~~~!!!77777!77777777?????????^....
// !777????JJJJJJ???7!!~~~~~~^^:::::........:^~^^^^~7Y5?^^~~~^^^?YY7!77^:::::...:~7??77777777777!!~~~~~~~!!!~~^^^~~~~~~~~~~~~~~~~!!!!777777777!~:::...!!7!!!!~~~~~~~~~~~~~~!!!!77777777777777?????????^....
// !???JJJJJJJJJJJ??777!!!!!~~~~~777::.......::^^~~~~?J?!~77?~^~!?7!!!77~^::::::.:~7?77777777777!!~~~~~~~!!~~^^^^^^~~~^^^~~~~~~~~!!!!!77777777?J:::...777!!!!~~~~~~~~~~~~~~!!!!!7777777777777?????????~....
// 7J?JYYYYYJJJJJJ??????77777!!!!7J?^:::::::::^^^~~~~~~~~~!!!~~~~!!!!!777~^^:::::::~??77777777!!!~~~~^^^~~~~~~~~^~~~~~~~~~~~~~~~~!!!!777777777?J.:^...!?7!!!!~~~~~~~~~~~~~~!!!!!7777777777777?????????~::..
// 7JJJJYYYYYYYJJJ???JJJJ?????777!!!~~^^^^^^^^^^^~~~~~~~~~~~~~!!!!!!777?7~:^^:::::::~7777777!!!!~~~~^^^^~~~~~~~~~~~~~~~~!?7!~~~~~!!!!777777777?7.:^...~77!!!!~~~~~~~~~~~~~~~~!!!!!77777777777?????????7~::.
// 7JJJJJJJYYYYJJJJJJYY5YJJJJJJ?777!!!!~~~^^^^^~~~~~~~~~~~~~~~!!!77777??7^::::::::::^!777777!!!~~~~~~~~~~~~~~~~^^~~~~~~^75P?~~~~~!!!!77777777~:....:::::~7!!!!~~~~~~~~^~~~~~~!!!!!!!!!7777777?????????7^:^:
// !?JYYJJJJJJYYJJJJJJYYYJJJJJJJ??7777!!!!~~~~~~~~~~~~~~~~~~~!!!77777???7^^::::::::::~!!!!!!!!!!~^:::::::^~~~~~~~~~~~~~~~?55!~~~!!!777777777~......^^^:..^!!!!~~~~~~^^^~~~~~~!!!!!!!!7777777??????????!:.::
// !?JYYJJJJJJYYYYYYYYYYYJJJJJJJ???777777!!!!!!!!!!~~~~~~~~!!!7777777???!^^^:::::^^::^!!!!!~^^^:...........:^~~~~~~~~~~~~~!7!~~!!!!777777777:....:::^^::.:!!!!~~~~~~^^~~~~~!!!!!!!!!!7777777??????????~....
// !???JJJJYYYYYYYYYYYYYJJJJJJJJJ???777777777!!!!!!!!!!!!!!!777777777??7^^^^^:::^~~^::^^^^::..................~~~~~~~~~~~~~!!!!!!!777777777?^...~!~~!?~...!!!!~~~~~~^~~~~~!!!!!!!!!!7777777???????????~....
// 7JJJYYYYYYYYYYYYYYYYYYJJJJJJJJ???777777777777777777!!!!!7777777777?7!~!!~^^::^~~~^:::::....................~^^^^^^~~~!!!777!!77777777777!:...!77777:...~!!!~~~~~~~~~~~!!!!!!!!!!!77777777??????????~:...
// J5555555555YYYYYYYYYYYYJJJJJJJJ???77777777777???7777!!!!!777777??????77!~~^^^^~!77!!!!~::..................::^^~~~~~7YY?!!!!!!!!!!!!!!!!!^...^~^^~!:...~!~~~~~^^^^~~~~~~~~!~~~!!!777777777????????7~:...
// J55555555555YYYYYYYYJJJJJJJJJJ?????7777777777777!!!!!!~~~!!7777????JJ?!~~^^^^~!7JJYYJJ7!~:....:...............^~~~~~!Y5J~~!!!!!!!!!!!~!~~:...^:...:...:~~~~^^^^::^^^~~~~~~~~~~~~!!!!777!!77!!!777!!!:...
// JPPPP55555YYYYJJJJJJJJJJJJ??????7777777!!!!!!!!!!~~~~~~~~~~!777???JJJJ7!~~~~~!!7JY5555YJJ7^:.:~~...............:^~~~~!!!~~~~~~~~~~!!!!~~~~::.........^~~^^^^^:::::^^~~~~~~~^^^~~~~~~~~~~~!!!!!!77!!!:...
// JYYYYYYYYYJJJJJ??????????77777!!!!!!!!!!!!~~~~~~~~~~~~^^~~~!!7???JJJJJJ?7!~~~~!77?JY55YYY?~::::~^..............::^^^^^^^^^^^~~~~~~~~~~~~~~~^::.....:^~~^^^:::::::::^^~~~~~~~~~~~~~~~~~~~~!!!!!!!!!!^....
// !?7777???????7777777777!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!7?JJJJJJ?????!~^^~~!!~!????JJJ?!^:::^~............:!?^:^^^^^^^^^~~~~~~~~~~~~~~~~^::...:^^^^^::::::::::::^^~~~~~~~~~~~~~~~~~~~!!!!!!!!!!^....
// !7777!7777777777!!!!!!!!!~~~~~~^~~~~~~~~~~~~~^~~~~~~~~~~~!!7??JJJJJ?77??7!~^~~~^^^~!~~^^~~~~!~^^:^^...........::::^^^^^^^^^^^^^~~~~~~~~~~~^^:.....:^^::::::::...::::^^^~~~~~~~~~~~~~~~~~~!!!!!!!!!~:....
// !7777!7!!77!7!!7!!!!!!!!!!~~~~~~^^~~~~~~~~~~~^~~~^^^^~~!!7?JJJJJ???77777!~~~~~^^:^~~^^:::::^^~~~~^:.........:::..^^~~^^^^^^^^^^~~~~~~~~~~~^^:..::::^^^^:::::::::::::^^^^^^~~~~~~~~~~~~~~~~~~!!!!!!~:....
// !7777777777!!!!!!!!!!!!!!!~~~~~~~~~~~~~~~~~~^^~~~^^^~~!!7?Y55Y????????77!~!~~~~~^^^^^^:::::^^^~~~^^::::^:....::::^~~~~~~~~~~~~~~~~~~~~~~~~^::..::::^^^^^^^:::::::::^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~^:....
// !7777777777777777777!!!!!!!~~~~~~~~~~~~~~~~~^^^^^^~~!7?J???JJ?7777777777!^^^^~!!!~^^:::::::::::::^^^^:^~::...:~^:~~~~~~~~~~~~~~~~~~~~~~~~^:....:^^^^.:^^^^^::::::^^^^^^^^^^^^^^^^^~~~~~~~!!!!!!!!~~^::..
// !77777777777777777777!!!!!!!!!!!~~~~~~~~~~~~~~~~^~~!?JYJ?!!!!~~~!!!7777!~^:^^^~!~~^::::::::::::::^^^^:::::...::^~~~!!!!!!!!!~~~~~~~~~~~~~:.....::^^::.^~^^^^^:::^^^^^^^^^^^^^^^^^^^~~~~~!~!!!!!!!!~~^:..
// !7777777777777777777!!!!!!!!!!!~~~~~~~~~!!!!!!~~~~~!7777!~~~~~~~!!!7777!~^::^^~!7~^:::::^^:::::::::::::::::::::~77!!!!!!!!!!!!!!!!!!~~~~~:..:^^:^~^::.:^^^^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~!!!!!!!!!!~^^^:
// !77777777777777777!!!!!!!!!!~!!!!!!!!!!!!!!!!!!!!!!!!!!!~~~!!!!!!7777777~^:^^^~??7!^^:^~~^::::::^::::^^^^^^^^^^~!!!!!7777!!!!!!!!!!!!!!!~::.:~!~~~^::.:^^^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~!!!!!!!!!!!^:^^:
// !?7777777777777777!!!!!!7!!!!!!!7777777777777777777!!!!!!!!!!777777??JJ?^:^^^^~7JYJ?!~!!~^::::^!^:^^^^^^~~~~~~~~!!!!7777777!!!!!!!!!!!!!~::::^!~~~~:.:^~^^^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~!!!!!!!!!!!::...
// 7????????7777777777!!!!77777777777777777???7777777777777777777777???777~^~!!~~~?Y555YJ7~~~^^^^~~^^^^~~~~~!!!!!!!7777777777777!!!!!!7777!7^::::::::^:.:^~^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~!!!!!!!!!7!::..:
// 7JJ?JJJJ??????????777777??????????????????????????????77777777???!~~^^!7JYYYJ?YPPP555J7~~~~^^^^^~~~~!!!!!!7!!7??????????????777777777777?!::::..::...^!!~~~~~~~!!!!!!!!!!777777??????????????77???7~:...
// 7JJJJJJJJJJJJJJJJJJJ??????JJJ???????JJJJJJJJJ???JJJ???????????JJ?~^^^~7JY5555GBBGPPPPY7~~~^^^^~~~~!!!!!!7777JYYYJJJJYJJJJJJJJJJJJ????????J!~^:::::::~7?77??????????????JJJJJJJJJJJJJJJJJJJJJJJ??J??~::..
// ?YYJJYYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJYYYYJ~:^~7?Y5PGB&&#P55PP5J7777!~^^~~~~!!!!777JY555YYYY55YYYYYYYJJJJJJJJJJJYYYYJ!^^::::7JJJJJJJJJJJJJJJJJYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJ7^:::
// ?YYYYYYY5555555555YYYYYYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJJJJJJJJYY55555J7!!7J5P#&&@#555PPPP555YY?7!~~~~~!7JY5555555555555555555YYYYYYYYYYYYYYYYY7^^^^::!YYYYYYYYYYYYYYYYY555555555555YYYYYYYYYYYYYYYYYYJ^:::
// J555555555555555555555555555555555YYYYYYYYYYYYYYYYYYYYYYYYYYYY55555P57~7J5G&@@@BP55PPPPGPP55YJ?7!!!77Y55555555555555555555555555555555555555?::^^^^!5YYYYYYYYYY55555555555555555555555555555555Y55Y!::::
// J55555555555555555555555555555555555555555YYYYYYYYYYYYYYYYYYY555PPPPY??Y5P#&@@&GPPPPGGGGPP5YJJJ?7!!JPPP5555555555555555555555555555555555555!:::^^^~?555555555555555555555555555555555555555555555Y^.:::
// Y55555555P55555555555555555555555555555555555555YY55YYYY5YY5555PPPPPPPPP5B&&&&BGPPGGGGGGPP55YYYJ7!7?J7?Y5PPPP555555555555555PPPPPPPPPPP555?^::::^^~^:~Y55555555555555PPPPPPPPPPP555555555555555555J^:::~
// YPPP55PPPPPPPPPPPPP5555PPPPP555555555555555555555555555555555555PPPPPPGPP#&&&BPPGGGGGGGGGPPPP5J7!!!!!!!!!?Y5PPPPPPPPPPPPPPPPPPPPPPPPPPPPPJ^:^^:^^~~~^:~5PP55555PPPPPPPPPPPPPPPPPPPPPPPPP5555555555?^:::!
// YPPPPPPPPPPPPPPPPPPPPPPPPPPPP55P555555555555555555555555555555555PPP5YYY5B&&&PY5PGGGGGGGGPPP5Y?!!!!!7!!!!~~~!7J55555PPPPPPPPPPPPPPPPPPPPP7^^^~!^^!7!^^^YPPPP55PPPPPPPPPPPPPPP5?777?JJJJJJYYYY55555J^:::~
// YPPPPPPPPPPPPPPPPPPPPPPPPPP555PPP5555555555555555555555555555555555J7777?Y###Y?YPPPPGGGPPP555?7!!~~!7!!!!~~^^^^~J55PPPPPPPPPPPPPPPPPPPPPP7^^^7PYJPP!^^~YPPPPPPPPPPPPPPPPPPPP57^^^^^^^^^^^^^^~!?YYY?~^^::
// YPPPPPPPPPPPPPPPPPPPPPPPPPP555555PP55555555555555555555555555555555J7!!!!7G#B??5PPPPGGGP5YJYJ7!!~~~!!!!!~~~^^^!J5PPPPPPPPPPPPPPPPPPPPPPPP?^~^!PPPPP!^~~YPPPPPPPPPPPPPPPPPPP55?~~~~~~~~~^^^^^^^^^^^^~~^^:
// YPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5P55555555555555555555555555555555Y?77!!!P#B??5PPPPGPP5J?77!!~~~~~~~!!~~^^^~75PPPPPPPPPPPPPPPPPPPPPPPPPPJ~~~~!~^!Y!^~~5PPPPPPPPPPPPPPPPPPP55Y!~~~~~~~~~~~~^^^^^^^^~~~~^
// 5PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP55555555555PPPPP5555555555PPPP5?77!!7PBBY7JPPPPPPYJ?!!!~~~~~~~~!!!!~~~!J5PPPPPPPPPPPPPPPPPPPPPPPPPPPY~!~~^^^^^^^~?PPPPPPPPPPPPPPPPPPPP555J?7~~~~~~~~~~^^^^^^^^~~!~~
// 5PPPPPPGGGGGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5555PPPPPPPPPPPPPP5555PPP57!7777PBBPYJ5PPPP5YJ7~~~~~~~!!~!7777777JPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPJ!~~~^^^~~!JPPPPPPPPPPPPPPPPPPPPPPPPPP5?7!~~~~~~~~^^^^^^^~~~~~
// 5PPPPPPPGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP555Y7!777?5GP55?Y555YYJ?!!!~~~~~!!!!7777777J55PPPPPPPPPPPPPPPPPPPPPPPPPPPPP5?~~~~~~75PPPPPPPPPPPPPPPPPPPPPPPPPPP55Y7~~~~~~~~^^^^^^~~~~~~
// 5GGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5PPPPPPP55?777777PG5J5?JYYJJJJ?!~~~~~~~~!!!777777!?PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPGY!!!!!!JGPGPPPPPPPPPPPPPPPPPPPPPPPPPPP557!~~^^^^^^^^^^~~^^^^
void terminatePaczka(const char* uuid) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/terminate", data_json, headers, &terminate_paczka, false);
    cJSON_Delete(data);
    free(data_json);
}

// sprawdź czy skrytka została actually zamknięta
void jaktamSkrytka(const char* uuid, bool otwarta) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    if (otwarta) {
        cJSON_AddItemToObject(data, "expectedStatus", cJSON_CreateString("OPENED"));
    } else {
        cJSON_AddItemToObject(data, "expectedStatus", cJSON_CreateString("CLOSED"));
    }

    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/status", data_json, headers, &jak_tam_skrytka, false);
    cJSON_Delete(data);
    free(data_json);
}

void getPaczkomatImage(const char* url) {
    if (!url || strlen(url) == 0) return;

    
    obrazekdone = false;

    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    
    
    queue_request(url, NULL, headers, &paczkomat_image, true);
}



void getEverything() {
    doing_debug_logs = true;
    CURL *debug_curl = curl_easy_init();
    log_to_file("LOGI ZBIORCZE (DEBUG PURPOSES ONLY)\n");
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", authToken);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authheader);
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    free(authheader);
    log_to_file("[API] Paczki do ciebie:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas);
    log_to_file("[RESPONSE] %s\n", paczkas.data);
    log_to_file("[API] Paczki od ciebie:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas);
    log_to_file("[RESPONSE] %s\n", sent_paczkas.data);
    log_to_file("[API] Paczki zwrócone:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas);
    log_to_file("[RESPONSE] %s\n", returned_paczkas.data);
    log_to_file("[API] Dane użytkownika:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera);
    log_to_file("[RESPONSE] %s\n", dane_usera.data);
    doing_debug_logs = false;
    curl_slist_free_all(headers);
    curl_easy_cleanup(debug_curl);
}