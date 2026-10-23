#include <3ds.h>
#include <citro2d.h>
#include "inpost_api.h"
#include "../scenes/scene_manager.h"
#include "../main.h"
#include "../utils/cwav_shit.h"
#include "../utils/sprites.h"
#include "../utils/text.h"
#include <math.h>
#include <stdio.h>
#include <jansson.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "../utils/request.h"
#include "../libs/cJSON.h"
#include "../utils/utils.h"
#include "../utils/lang.h"
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
ResponseBuffer paczkomat_search = {NULL, 0, 0, false};
ResponseBuffer cena_paczkas = {NULL, 0, 0, false};
ResponseBuffer wysylka_request = {NULL, 0, 0, false};
ResponseBuffer testreq = {NULL, 0, 0, false};
ResponseBuffer status_wysylki = {NULL, 0, 0, false};
ResponseBuffer blik_pay = {NULL, 0, 0, false};
ResponseBuffer auth_qr_verifier = {NULL, 0, 0, false};

char* refreshToken = NULL;
char* authToken = NULL;
int inpointsy = 0;

bool pers_data_done;


Paczka paczka_list[MAX_PACZKAS];
Paczka paczka_list_sent[MAX_PACZKAS];

int paczka_count_sent = 0;
int paczka_count = 0;

User uzytkownik = {0};

//lubie ten endpoint bo se moge czas sprawdzić w szkole xD (naprawcie kiedyś czas w systemie plz)
void testrequest(){
    queue_request("https://zabka-snrs.zabka.pl/v4/server/time", NULL, NULL, &testreq, false, GET);
}

// wyślij prośbe o kod sms na podany numer
void sendNumerTel(char numertel[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[128]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"}}", numertel);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account", request, headers, &send_phone_numer, false, POST);
}

// wyślij kod sms z wcześniej dostanego sms'a
void sendKodSMS(char numertel[60], char kodsms[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[256]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"},\"smsCode\":\"%s\",\"devicePlatform\":\"Android\"}", numertel, kodsms);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account/verification", request, headers, &send_kod_sms, false, POST);
}

// odśwież token używając refresha (tego co dostawałeś przy logowaniu bądz po odświeżeniu)
void refresh_the_Token(char* refresh) {
    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    char request[256]; 
    snprintf(request, sizeof(request), "{\"refreshToken\":\"%s\"}", refresh);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/authenticate", request, headers, &get_refresh_token, false, POST);

    curl_slist_free_all(headers);
}

void parseRefreshedToken(const char* json)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        log_to_file("[parseRefreshedToken] parse failed");
        return;
    }

    cJSON *accesstoken  = cJSON_GetObjectItemCaseSensitive(root, "access_token");
    cJSON *refreshtoken = cJSON_GetObjectItemCaseSensitive(root, "refresh_token");

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

void generate_uuidv4(char *out) {
    unsigned char b[16];
    for (int i = 0; i < 16; i++) {
        b[i] = (unsigned char)(rand() % 256);
    }

    b[6] = (b[6] & 0x0f) | 0x40;
    b[8] = (b[8] & 0x3f) | 0x80;

    snprintf(out, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0], b[1], b[2], b[3],
        b[4], b[5],
        b[6], b[7],
        b[8], b[9],
        b[10], b[11], b[12], b[13], b[14], b[15]);
}

// wyślij dane z qrki do serwera by uzyskać token
void sendAuthQR(const char *verifier, const char *code) {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "x-app-id: pl.inpost.inmobile");
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);

    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.19.0(41900000) (Android 9; SM-A600FN; samsung a6lte; pl)");

    char request[1024]; 
    snprintf(request, sizeof(request),
        "client_id=inpost-mobile"
        "&grant_type=authorization_code"
        "&redirect_uri=https%%3A%%2F%%2Faccount.inpost-group.com%%2Fcallback"
        "&code_verifier=%s"
        "&code=%s",
        verifier, code);

    queue_request("https://api-inmobile-pl.easypack24.net/global/oauth2/token", request, headers, &auth_qr_verifier, false, POST);
}

// sparsuj dane z jsona ze strony 
void getTokenFromQR(const char* json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        log_to_file("[getTokenFromQR] parse failed");
        return;
    }

    cJSON *verifier = cJSON_GetObjectItemCaseSensitive(root, "verifier");
    cJSON *code     = cJSON_GetObjectItemCaseSensitive(root, "code");

    const char *verifier_str = cJSON_IsString(verifier) ? cJSON_GetStringValue(verifier) : NULL;
    const char *code_str     = cJSON_IsString(code)     ? cJSON_GetStringValue(code)     : NULL;

    if (verifier_str && code_str) {
        sendAuthQR(verifier_str, code_str);
    } else {
        log_to_file("[getTokenFromQR] verifier or code not found in JSON");
    }

    cJSON_Delete(root);
}

// pobierz paczki
void getPaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "x-app-id: pl.inpost.inmobile");
    switch(currentLang) {
        case PL:
            headers = curl_slist_append(headers, "Accept-Language: pl-PL");
            break;
        case EN:
            headers = curl_slist_append(headers, "Accept-Language: en-US");
            break;
        default:
            headers = curl_slist_append(headers, "Accept-Language: pl-PL");
            break;
    }

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas, false, GET);
}

// (używane w dev buildach) parsuj test dane
void parseFakePaczkas(bool isSent) {
    bool ignore_bo_kurier = false;

    cJSON *root = NULL;
    open_json(isSent ? "romfs:/test_sent_data.json" : "romfs:/test_data.json", &root);

    cJSON *parcels = cJSON_GetObjectItemCaseSensitive(root, "parcels");
    if (!cJSON_IsArray(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        cJSON_Delete(root);
        return;
    }

    paczka_count = 0;
    paczka_count_sent = 0;

    size_t total_parcels = cJSON_GetArraySize(parcels);
    
    for (size_t i = total_parcels; i > 0; i--) {
        cJSON *parcel = cJSON_GetArrayItem(parcels, i - 1);
    
        if (isSent) {
            if (paczka_count_sent >= MAX_PACZKAS)
                break;
        } else {
            if (paczka_count >= MAX_PACZKAS)
                break;
        }

        Paczka *p = NULL;
        if (isSent) {
            p = &paczka_list_sent[paczka_count_sent];
        } else {
            p = &paczka_list[paczka_count];
        }
        cJSON *orderID = cJSON_GetObjectItemCaseSensitive(parcel, "orderId");
        if (isSent && !cJSON_IsString(orderID)) {
            continue;
        }
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
            if (!isSent) {
                jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                        cJSON_GetObjectItemCaseSensitive(parcel, "pickUpDate"));     
                jstrcpy(p->opencode, sizeof(p->opencode),
                        cJSON_GetObjectItemCaseSensitive(parcel, "openCode"));
                if (cJSON_GetObjectItemCaseSensitive(parcel, "openCode")) {
                    p->paczka_openable = true;
                } else {
                    p->paczka_openable = false;
                }
            } else{
                jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                        cJSON_GetObjectItemCaseSensitive(parcel, "confirmationDate"));     
                jstrcpy(p->opencode, sizeof(p->opencode),
                        cJSON_GetObjectItemCaseSensitive(parcel, "quickSendCode"));
                if (cJSON_GetObjectItemCaseSensitive(parcel, "quickSendCode")) {
                    p->paczka_openable = true;
                } else {
                    p->paczka_openable = false;
                }
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
            cJSON *pickup = NULL;
            if (!isSent) {
                pickup = cJSON_GetObjectItemCaseSensitive(parcel, "pickUpPoint");
            } else {
                pickup = cJSON_GetObjectItemCaseSensitive(parcel, "dropOffPoint");
            }
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

        if (isSent) {
            paczka_count_sent++;
        } else {
            paczka_count++;
        }
    }
    if (isSent) {
        log_to_file("[parsePaczkas] Parsed %d sent paczkas", paczka_count_sent);
    } else {
        log_to_file("[parsePaczkas] Parsed %d received paczkas", paczka_count);
    }

    cJSON_Delete(root);
}

// parsuj paczki do structa "paczka_list"
void parsePaczkas(const char* json, bool isSent) {
    bool ignore_bo_kurier = false;

    cJSON *root = cJSON_Parse(json);
    

    cJSON *parcels = cJSON_GetObjectItemCaseSensitive(root, "parcels");
    if (!cJSON_IsArray(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        cJSON_Delete(root);
        return;
    }

    paczka_count = 0;
    paczka_count_sent = 0;

    size_t total_parcels = cJSON_GetArraySize(parcels);
    
    for (size_t i = total_parcels; i > 0; i--) {
        cJSON *parcel = cJSON_GetArrayItem(parcels, i - 1);
    
        if (isSent) {
            if (paczka_count_sent >= MAX_PACZKAS)
                break;
        } else {
            if (paczka_count >= MAX_PACZKAS)
                break;
        }

        Paczka *p = NULL;
        if (isSent) {
            p = &paczka_list_sent[paczka_count_sent];
        } else {
            p = &paczka_list[paczka_count];
        }
        cJSON *orderID = cJSON_GetObjectItemCaseSensitive(parcel, "orderId");
        if (isSent && !cJSON_IsString(orderID)) {
            continue;
        }
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
            if (!isSent) {
                jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                        cJSON_GetObjectItemCaseSensitive(parcel, "pickUpDate"));     
                jstrcpy(p->opencode, sizeof(p->opencode),
                        cJSON_GetObjectItemCaseSensitive(parcel, "openCode"));
                if (cJSON_GetObjectItemCaseSensitive(parcel, "openCode")) {
                    p->paczka_openable = true;
                } else {
                    p->paczka_openable = false;
                }
            } else{
                jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                        cJSON_GetObjectItemCaseSensitive(parcel, "confirmationDate"));     
                jstrcpy(p->opencode, sizeof(p->opencode),
                        cJSON_GetObjectItemCaseSensitive(parcel, "quickSendCode"));
                if (cJSON_GetObjectItemCaseSensitive(parcel, "quickSendCode")) {
                    p->paczka_openable = true;
                } else {
                    p->paczka_openable = false;
                }
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
            cJSON *pickup = NULL;
            pickup = cJSON_GetObjectItemCaseSensitive(parcel, "pickUpPoint");
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

        if (isSent) {
            paczka_count_sent++;
        } else {
            paczka_count++;
        }
    }
    if (isSent) {
        log_to_file("[parsePaczkas] Parsed %d sent paczkas", paczka_count_sent);
    } else {
        log_to_file("[parsePaczkas] Parsed %d received paczkas", paczka_count);
    }

    cJSON_Delete(root);
}

void getWyslanePaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas, false, GET);

}



// pobierz zwrócone paczki
void getReturnedPaczkas() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas, false, GET);

}

// pobierz dane jak imie i nazwisko, ulubiony paczkomat itp
void getPersonalData() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera, false, GET);
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

void readPersonalData(const char *json_str)
{
    if (!json_str)
        return;

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return;
    }

    cJSON *item;

    item = cJSON_GetObjectItemCaseSensitive(root, "firstname");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.firstname = strdup(item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "lastname");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.lastname = strdup(item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "phone_number");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.phone_number = strdup(item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "phoneprefix");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.phoneprefix = strdup(item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "email");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.email = strdup(item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "id_pref_paczkomatu");
    if (cJSON_IsString(item) && item->valuestring)
        uzytkownik.id_pref_paczkomatu = strdup(item->valuestring);

    cJSON *adres_array =
        cJSON_GetObjectItemCaseSensitive(root, "adres_pref_paczkomatu");

    if (cJSON_IsArray(adres_array)) {
        uzytkownik.adres_count = cJSON_GetArraySize(adres_array);

        if (uzytkownik.adres_count > 0) {
            uzytkownik.adres_pref_paczkomatu =
                malloc(uzytkownik.adres_count * sizeof(char *));

            if (uzytkownik.adres_pref_paczkomatu) {
                for (int i = 0; i < uzytkownik.adres_count; i++) {
                    cJSON *adres_item =
                        cJSON_GetArrayItem(adres_array, i);

                    if (cJSON_IsString(adres_item) &&
                        adres_item->valuestring) {
                        uzytkownik.adres_pref_paczkomatu[i] =
                            strdup(adres_item->valuestring);
                    } else {
                        uzytkownik.adres_pref_paczkomatu[i] = NULL;
                    }
                }
            }
        }
    }
    log_to_file("[readPersonalData] Read personal data: %s %s, phone: %s%s, email: %s, preferred locker ID: %s",
                uzytkownik.firstname ? uzytkownik.firstname : "N/A",
                uzytkownik.lastname ? uzytkownik.lastname : "N/A",
                uzytkownik.phoneprefix ? uzytkownik.phoneprefix : "N/A",
                uzytkownik.phone_number ? uzytkownik.phone_number : "N/A",
                uzytkownik.email ? uzytkownik.email : "N/A",
                uzytkownik.id_pref_paczkomatu ? uzytkownik.id_pref_paczkomatu : "N/A");

    cJSON_Delete(root);
}



// pobierz inpointsy (never used lol)
void getInPointsBalance() {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/loyalty/v1/wallet/balance", NULL, headers, &balans_konta, false, GET);
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

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
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
    queue_request("https://api-inmobile-pl.easypack24.net/v2/collect/validate", data_json, headers, &validate_paczkomat, false, POST);
    cJSON_Delete(data);
    free(data_json);
}

// sezamie otwórz się
void openPaczkomat(const char* uuid) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/open", data_json, headers, &open_paczkomat, false, POST);
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

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/terminate", data_json, headers, &terminate_paczka, false, POST);
    cJSON_Delete(data);
    free(data_json);
}

// sprawdź czy skrytka została actually zamknięta
void jaktamSkrytka(const char* uuid, bool otwarta) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sessionUuid", cJSON_CreateString(uuid));
    if (otwarta) {
        cJSON_AddItemToObject(data, "expectedStatus", cJSON_CreateString("OPENED"));
    } else {
        cJSON_AddItemToObject(data, "expectedStatus", cJSON_CreateString("CLOSED"));
    }

    char *data_json = cJSON_Print(data);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/status", data_json, headers, &jak_tam_skrytka, false, POST);
    cJSON_Delete(data);
    free(data_json);
}

void getPaczkomatImage(const char* url) {
    if (!url || strlen(url) == 0) return;

    
    obrazekdone = false;

    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    
    
    queue_request(url, NULL, headers, &paczkomat_image, true, GET);
}

void searchPaczkomat(const char* paczkomat_id) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[searchPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    char url[256];
    snprintf(url, sizeof(url), "https://api-inmobile-pl.easypack24.net/v4/points?query=%s&filter=c2x-target&country=PL&perPage=100", paczkomat_id);
    queue_request(url, NULL, headers, &paczkomat_search, false, GET);
}

// jest też /v3/prices/parcels?sourceCountryCode=PL ale for some fucking reason robi to samo co ten xDDD
void getWysylkaCena(){
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getWysylkaCena] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "device-uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v2/prices/parcels?sourceCountryCode=PL", NULL, headers, &cena_paczkas, false, GET);
}
PaczkomatPoint paczkomat_search_results[MAX_PACZKOMAT_RESULTS];
int paczkomat_search_count = 0;

void parsePaczkomatSearch(const char* json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        log_to_file("[parsePaczkomatSearch] parse failed");
        return;
    }

    cJSON *points = cJSON_GetObjectItemCaseSensitive(root, "points");
    if (!cJSON_IsArray(points)) {
        log_to_file("[parsePaczkomatSearch] points is not an array");
        cJSON_Delete(root);
        return;
    }

    paczkomat_search_count = 0;
    size_t total_points = cJSON_GetArraySize(points);

    for (size_t i = 0; i < total_points && paczkomat_search_count < MAX_PACZKOMAT_RESULTS; i++) {
        cJSON *point = cJSON_GetArrayItem(points, i);
        PaczkomatPoint *p = &paczkomat_search_results[paczkomat_search_count];

        jstrcpy(p->name, sizeof(p->name), cJSON_GetObjectItemCaseSensitive(point, "name"));
        jstrcpy(p->locationDescription, sizeof(p->locationDescription), cJSON_GetObjectItemCaseSensitive(point, "locationDescription"));

        cJSON *address = cJSON_GetObjectItemCaseSensitive(point, "addressDetails");
        if (cJSON_IsObject(address)) {
            jstrcpy(p->city, sizeof(p->city), cJSON_GetObjectItemCaseSensitive(address, "city"));
            jstrcpy(p->street, sizeof(p->street), cJSON_GetObjectItemCaseSensitive(address, "street"));
            jstrcpy(p->buildingNumber, sizeof(p->buildingNumber), cJSON_GetObjectItemCaseSensitive(address, "buildingNumber"));
        }

        paczkomat_search_count++;
    }

    log_to_file("[parsePaczkomatSearch] Parsed paczkomat search points");
    cJSON_Delete(root);
}

float parcelA_price, parcelB_price, parcelC_price, parcelD_price;
int parcelA_width, parcelB_width, parcelC_width, parcelD_width;
int parcelA_height, parcelB_height, parcelC_height, parcelD_height;
int parcelA_length, parcelB_length, parcelC_length, parcelD_length;
int parcelA_weight, parcelB_weight, parcelC_weight, parcelD_weight;

void parseWysylkaCena(const char* json){
    cJSON *root = cJSON_Parse(json);
    cJSON *countries = cJSON_GetObjectItemCaseSensitive(root, "countries");
    cJSON *country = cJSON_GetArrayItem(countries, 0);
    cJSON *boxMachineService = cJSON_GetObjectItemCaseSensitive(country, "boxMachineService");
    cJSON *parcelA = cJSON_GetObjectItemCaseSensitive(boxMachineService, "parcelA");
    cJSON *priceA = cJSON_GetObjectItemCaseSensitive(parcelA, "price");
    cJSON *amountA = cJSON_GetObjectItemCaseSensitive(priceA, "amount");
    cJSON *dimensionsA = cJSON_GetObjectItemCaseSensitive(parcelA, "dimensions");
    cJSON *weightA = cJSON_GetObjectItemCaseSensitive(dimensionsA, "weight");
    cJSON *widthA = cJSON_GetObjectItemCaseSensitive(dimensionsA, "width");
    cJSON *heightA = cJSON_GetObjectItemCaseSensitive(dimensionsA, "height");
    cJSON *lengthA = cJSON_GetObjectItemCaseSensitive(dimensionsA, "length");
    parcelA_width = cJSON_GetNumberValue(widthA);
    parcelA_height = cJSON_GetNumberValue(heightA);
    parcelA_length = cJSON_GetNumberValue(lengthA);
    parcelA_weight = cJSON_GetNumberValue(weightA);
    parcelA_price = cJSON_GetNumberValue(amountA);

    cJSON *parcelB = cJSON_GetObjectItemCaseSensitive(boxMachineService, "parcelB");
    cJSON *priceB = cJSON_GetObjectItemCaseSensitive(parcelB, "price");
    cJSON *amountB = cJSON_GetObjectItemCaseSensitive(priceB, "amount");
    cJSON *dimensionsB = cJSON_GetObjectItemCaseSensitive(parcelB, "dimensions");
    cJSON *weightB = cJSON_GetObjectItemCaseSensitive(dimensionsB, "weight");
    cJSON *widthB = cJSON_GetObjectItemCaseSensitive(dimensionsB, "width");
    cJSON *heightB = cJSON_GetObjectItemCaseSensitive(dimensionsB, "height");
    cJSON *lengthB = cJSON_GetObjectItemCaseSensitive(dimensionsB, "length");
    parcelB_width = cJSON_GetNumberValue(widthB);   
    parcelB_height = cJSON_GetNumberValue(heightB);
    parcelB_length = cJSON_GetNumberValue(lengthB);
    parcelB_weight = cJSON_GetNumberValue(weightB);
    parcelB_price = cJSON_GetNumberValue(amountB);

    cJSON *parcelC = cJSON_GetObjectItemCaseSensitive(boxMachineService, "parcelC");
    cJSON *priceC = cJSON_GetObjectItemCaseSensitive(parcelC, "price");
    cJSON *amountC = cJSON_GetObjectItemCaseSensitive(priceC, "amount");
    cJSON *dimensionsC = cJSON_GetObjectItemCaseSensitive(parcelC, "dimensions");
    cJSON *weightC = cJSON_GetObjectItemCaseSensitive(dimensionsC, "weight");
    cJSON *widthC = cJSON_GetObjectItemCaseSensitive(dimensionsC, "width");
    cJSON *heightC = cJSON_GetObjectItemCaseSensitive(dimensionsC, "height");
    cJSON *lengthC = cJSON_GetObjectItemCaseSensitive(dimensionsC, "length");
    parcelC_width = cJSON_GetNumberValue(widthC);
    parcelC_height = cJSON_GetNumberValue(heightC);
    parcelC_length = cJSON_GetNumberValue(lengthC);
    parcelC_weight = cJSON_GetNumberValue(weightC);
    parcelC_price = cJSON_GetNumberValue(amountC);

    cJSON *parcelD = cJSON_GetObjectItemCaseSensitive(boxMachineService, "parcelD");
    cJSON *priceD = cJSON_GetObjectItemCaseSensitive(parcelD, "price");
    cJSON *amountD = cJSON_GetObjectItemCaseSensitive(priceD, "amount");
    cJSON *dimensionsD = cJSON_GetObjectItemCaseSensitive(parcelD, "dimensions");
    cJSON *weightD = cJSON_GetObjectItemCaseSensitive(dimensionsD, "weight");
    cJSON *widthD = cJSON_GetObjectItemCaseSensitive(dimensionsD, "width");
    cJSON *heightD = cJSON_GetObjectItemCaseSensitive(dimensionsD, "height");
    cJSON *lengthD = cJSON_GetObjectItemCaseSensitive(dimensionsD, "length");
    parcelD_width = cJSON_GetNumberValue(widthD);
    parcelD_height = cJSON_GetNumberValue(heightD);
    parcelD_length = cJSON_GetNumberValue(lengthD);
    parcelD_weight = cJSON_GetNumberValue(weightD);
    parcelD_price = cJSON_GetNumberValue(amountD);

    cJSON_Delete(root);
}
RecieverInfo reciever_info = {0};
void submitWysylkaRequest(RecieverInfo reciever)
{
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[submitWysylkaRequest] ERROR: malloc failed");
        return;
    }

    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "Device-Uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(
        headers,
        "User-Agent: InPost-Mobile/4.18.0(41800002) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)"
    );
    headers = curl_slist_append(headers, "X-App-Id: pl.inpost.inmobile");
    headers = curl_slist_append(headers, authheader);

    cJSON *data = cJSON_CreateObject();
    if (!data) {
        free(authheader);
        curl_slist_free_all(headers);
        return;
    }

    cJSON_AddStringToObject(data, "deliveryType", "BOX_MACHINE");
    

    char priceStr[32];
    switch (reciever.parcelSize) {
        case 1:
            cJSON_AddStringToObject(data, "parcelSize", "A");
            reciever.parcelPrice = parcelA_price;
            break;
        case 2:
            cJSON_AddStringToObject(data, "parcelSize", "B");
            reciever.parcelPrice = parcelB_price;
            break;
        case 3:
            cJSON_AddStringToObject(data, "parcelSize", "C");
            reciever.parcelPrice = parcelC_price;
            break;
        case 4:
            cJSON_AddStringToObject(data, "parcelSize", "D");
            reciever.parcelPrice = parcelD_price;
            break;
        default:
            break;
    }
    snprintf(priceStr, sizeof(priceStr), "%.2f", reciever.parcelPrice);
    cJSON_AddStringToObject(data, "price", priceStr);

    cJSON *sender = cJSON_CreateObject();
    cJSON_AddStringToObject(sender, "firstName", uzytkownik.firstname);
    cJSON_AddStringToObject(sender, "lastName", uzytkownik.lastname);
    cJSON_AddStringToObject(sender, "email", uzytkownik.email);
    cJSON_AddItemToObject(data, "sender", sender);

    cJSON *receiver = cJSON_CreateObject();

    cJSON_AddStringToObject(receiver, "firstName", reciever.firstName);
    cJSON_AddStringToObject(receiver, "lastName", reciever.lastName);
    cJSON_AddStringToObject(receiver, "email", reciever.email);

    cJSON *phoneNumber = cJSON_CreateObject();
    cJSON_AddStringToObject(phoneNumber, "prefix", "+48");
    cJSON_AddStringToObject(phoneNumber, "value", reciever.phoneNumber);
    cJSON_AddItemToObject(receiver, "phoneNumber", phoneNumber);

    cJSON_AddItemToObject(data, "receiver", receiver);

    cJSON *deliveryPoint = cJSON_CreateObject();
    cJSON_AddStringToObject(
        deliveryPoint,
        "boxMachineName",
        reciever.boxMachineName
    );
    cJSON_AddItemToObject(data, "deliveryPoint", deliveryPoint);

    cJSON_AddStringToObject(data, "paymentMethod", "BLIK_CODE");
    cJSON_AddStringToObject(data, "destinationCountryCode", "PL");
    cJSON_AddStringToObject(data, "sourceCountryCode", "PL");
    cJSON_AddStringToObject(data, "currency", "PLN");
    cJSON_AddStringToObject(data, "labelFormat", "QUICK_SEND_CODE");

    char *data_json = cJSON_PrintUnformatted(data);

    if (!data_json) {
        log_to_file("[submitWysylkaRequest] ERROR: cJSON_PrintUnformatted failed");
        cJSON_Delete(data);
        free(authheader);
        curl_slist_free_all(headers);
        return;
    }

    log_to_file(data_json);

    queue_request("https://api-inmobile-pl.easypack24.net/v5/parcels", data_json, headers, &wysylka_request, false, POST);

    cJSON_Delete(data);
    free(data_json);
}

void parseWysylkaRequest(const char* json){
    cJSON *root = cJSON_Parse(json);
    cJSON *orderId = cJSON_GetObjectItemCaseSensitive(root, "orderId");

    strncpy(
        reciever_info.parcelOrderId,
        cJSON_GetStringValue(orderId),
        sizeof(reciever_info.parcelOrderId) - 1
    );

    reciever_info.parcelOrderId[
        sizeof(reciever_info.parcelOrderId) - 1
    ] = '\0';
    cJSON_Delete(root);
}

void checkWysylkaState(RecieverInfo reciever) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getWysylkaCena] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "Device-Uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Android 9; SM-A600FN; samsung a6lte; pl)");
    headers = curl_slist_append(headers, "X-App-Id: pl.inpost.inmobile");
    headers = curl_slist_append(headers, authheader);

    free(authheader);
    char url[256];
    snprintf(url, sizeof(url), 
            "https://api-inmobile-pl.easypack24.net/izi/app/shopping/v1/orders/%s/payment-details?forceRefresh=false", 
            reciever.parcelOrderId);
    queue_request(url, NULL, headers, &status_wysylki, false, GET);
}

void parseWysylkaState(const char* json){
    cJSON *root = cJSON_Parse(json);
    cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");

    if (strcmp(cJSON_GetStringValue(status), "AUTHORIZED") == 0) {
        reciever_info.parcelPaid = true;
    } else {
        reciever_info.parcelPaid = false;
    }
    cJSON_Delete(root);
}

void payBLIK(){
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getWysylkaCena] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");
    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "Device-Uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Android 9; SM-A600FN; samsung a6lte; pl)");
    headers = curl_slist_append(headers, "X-App-Id: pl.inpost.inmobile");
    headers = curl_slist_append(headers, authheader);
    cJSON *data = cJSON_CreateObject();

    cJSON_AddItemToObject(data, "type", cJSON_CreateString("PAYMENT"));
    cJSON_AddItemToObject(data, "orderId", cJSON_CreateString(reciever_info.parcelOrderId));
    cJSON_AddItemToObject(data, "blikCode", cJSON_CreateString(reciever_info.blikCode));

    char *data_json = cJSON_Print(data);
    free(authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/izi/app/shopping/v1/payments/blik-code", data_json, headers, &blik_pay, false, POST);
    cJSON_Delete(data);
}

void getEverything() {
    doing_debug_logs = true;
    CURL *debug_curl = curl_easy_init();
    log_to_file("LOGI ZBIORCZE (DEBUG PURPOSES ONLY)\n");
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: Bearer ") + strlen(authToken) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: Bearer %s", authToken);
    headers = curl_slist_append(headers, "Accept-Language: pl-PL");

    char device_uid[37];
    generate_uuidv4(device_uid);

    char device_header[128];
    snprintf(device_header, sizeof(device_header), "Device-Uid: %s", device_uid);
    headers = curl_slist_append(headers, device_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authheader);
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/4.18.0(41800002) (Android 9; SM-A600FN; samsung a6lte; pl)");
    free(authheader);
    log_to_file("[API] Paczki do ciebie:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas, GET);
    log_to_file("[RESPONSE] %s\n", paczkas.data);
    log_to_file("[API] Paczki od ciebie:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas, GET);
    log_to_file("[RESPONSE] %s\n", sent_paczkas.data);
    log_to_file("[API] Paczki zwrócone:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas, GET);
    log_to_file("[RESPONSE] %s\n", returned_paczkas.data);
    log_to_file("[API] Dane użytkownika:\n");
    refresh_data(debug_curl, "https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera, GET);
    log_to_file("[RESPONSE] %s\n", dane_usera.data);
    doing_debug_logs = false;
    curl_slist_free_all(headers);
    curl_easy_cleanup(debug_curl);
}