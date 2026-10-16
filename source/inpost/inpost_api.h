#ifndef INPOST_H
#define INPOST_H

#include "../utils/request.h"

extern ResponseBuffer send_phone_numer;
extern ResponseBuffer send_kod_sms;
extern ResponseBuffer paczkas;
extern ResponseBuffer sent_paczkas;
extern ResponseBuffer returned_paczkas;
extern ResponseBuffer get_refresh_token;
extern ResponseBuffer dane_usera;
extern ResponseBuffer balans_konta;
extern ResponseBuffer validate_paczkomat;
extern ResponseBuffer open_paczkomat;
extern ResponseBuffer terminate_paczka;
extern ResponseBuffer jak_tam_skrytka;
extern ResponseBuffer paczkomat_image;
extern ResponseBuffer paczkomat_search;
extern ResponseBuffer cena_paczkas;
extern ResponseBuffer wysylka_request;
extern ResponseBuffer testreq;
extern ResponseBuffer auth_qr_verifier;
extern ResponseBuffer status_wysylki;
extern ResponseBuffer blik_pay;

extern char* refreshToken;
extern char* authToken;
extern int inpointsy;

extern bool pers_data_done;

#define MAX_PACZKAS 32
#define MAX_EVENTS 16

typedef struct {
    char *firstname;
    char *lastname;
    char *phone_number;
    char *phoneprefix;
    char *email;
    char *id_pref_paczkomatu;
    char **adres_pref_paczkomatu;
    int adres_count;
} User;

typedef struct {
    char name[64];
    char date[32];
} PaczkaEvent;

typedef struct {
    char shipmentNumber[32];
    char status[16];
    char statusGroup[16];
    char parcelSize[4];

    char storedDate[32];
    char pickUpDate[32];

    char pickupPointName[128];
    char city[64];
    char street[64];

    float latitude;
    float longitude;

    char phonePrefix[8];
    char phoneNumber[16];
    char opencode[8];
    
    char qrCode[128];
    char imageUrl[256]; 
    
    char latestEvent[64];
    bool courier_paczka;
    bool paczka_openable;
    int eventCount;
    PaczkaEvent events[MAX_EVENTS];
} Paczka;

typedef struct {
    char firstName[64];
    char lastName[64];
    char email[256];
    char phoneNumber[16];
    char boxMachineName[64];
    int parcelSize;
    float parcelPrice;
    char parcelOrderId[128];
    bool parcelPaid;
    char blikCode[16];
} RecieverInfo;

extern Paczka paczka_list[MAX_PACZKAS];
extern Paczka paczka_list_sent[MAX_PACZKAS];
extern int paczka_count;
extern int paczka_count_sent;

extern User uzytkownik;
void testrequest();
void sendNumerTel(char numertel[60]);
void sendKodSMS(char numertel[60],char kodsms[60]);
void refresh_the_Token(char* refresh);
void parseRefreshedToken(const char* json);
void sendAuthQR(const char *verifier, const char *code);
void getTokenFromQR(const char* json);
void getPaczkas();
void parseFakePaczkas(bool isSent);
void parsePaczkas(const char* json, bool isSent);
void getWyslanePaczkas();
void getReturnedPaczkas();
void getPersonalData();
void parsePersonalData(const char* json);
void readPersonalData(const char *json_str);
void getInPointsBalance();
void parseInPointsBalance(const char* json);
void getPaczkomatStatus(const char* shipmentNumber, const char* openCode, const char* recieverPhoneNumber, const char* recieverPhonePrefix, float latitude, float longitude);
void openPaczkomat(const char* uuid);
void terminatePaczka(const char* uuid);
void jaktamSkrytka(const char* uuid, bool otwarta);
void getPaczkomatImage(const char* url);
void searchPaczkomat(const char* paczkomat_id);
void getWysylkaCena();
void payBLIK();
#define MAX_PACZKOMAT_RESULTS 50

typedef struct {
    char name[32];
    char city[64];
    char street[64];
    char buildingNumber[16];
    char locationDescription[128];
} PaczkomatPoint;

extern PaczkomatPoint paczkomat_search_results[MAX_PACZKOMAT_RESULTS];
extern int paczkomat_search_count;

void parsePaczkomatSearch(const char* json);

extern float parcelA_price, parcelB_price, parcelC_price, parcelD_price;
extern int parcelA_width, parcelB_width, parcelC_width, parcelD_width;
extern int parcelA_height, parcelB_height, parcelC_height, parcelD_height;
extern int parcelA_length, parcelB_length, parcelC_length, parcelD_length;
extern int parcelA_weight, parcelB_weight, parcelC_weight, parcelD_weight;

void parseWysylkaCena(const char* json);
extern RecieverInfo reciever_info;
void submitWysylkaRequest(RecieverInfo reciever);
void parseWysylkaRequest(const char* json);
void checkWysylkaState(RecieverInfo reciever);
void parseWysylkaState(const char* json);

void getEverything();

#endif
