#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <bits/endian.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <map>
#include <vector>
#include <algorithm>
#include "mapping.h"

using namespace std;

typedef struct Header {
    uint16_t ID;
# if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t QR: 1;
    uint16_t OPCODE: 4;
    uint16_t AA: 1;
    uint16_t TC: 1;
    uint16_t RD: 1;
    uint16_t RA: 1;
    uint16_t Z: 3;
    uint16_t RCODE: 4;
# elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t RD: 1;
    uint16_t TC: 1;
    uint16_t AA: 1;
    uint16_t OPCODE: 4;
    uint16_t QR: 1;
    uint16_t RCODE: 4;
    uint16_t Z: 3;
    uint16_t RA: 1;
# else
#   error "Endian check failed"
#endif
    uint16_t QDCOUNT; // # entries in question section
    uint16_t ANCOUNT; // # RR in ans section
    uint16_t NSCOUNT; // # NS records
    uint16_t ARCOUNT; // # additional records
} Header;

typedef struct QS {
    // QNAME
    uint16_t QTYPE;
    uint16_t QCLASS;
} QS;

typedef struct RR {
    // NAME
    uint16_t TYPE;
    uint16_t CLASS;
    uint32_t TTL;
    uint16_t RDLENGTH;
    // RDATA
} __attribute((packed)) RR;

uint16_t convert_endian(uint16_t n) {
    return (n >> 8) | (n << 8);
}

uint32_t convert_endian32(uint32_t n) {
    return ((n & 0xff000000) >> 24)| ((n & 0xff0000) >> 8) | ((n & 0xff00) << 8) | ((n & 0xff) << 24);
}

typedef struct Domain {
    string domain_name, ttl, class_, type, data;
} Domain;

string to_comma(char* buf, int &cur) {
    string tmp;
    while (buf[cur] && buf[cur] != ',' && buf[cur] != '\n' && buf[cur] != '\r') {
        tmp += buf[cur];
        cur++;
    }
    cur++;
    return tmp;
}

bool cmp(const string lhs, const string rhs) {
    return lhs.length() < rhs.length();
}

map<string, vector<Domain*>> ans_table;
vector<string> authorized(0);

Domain* find_rr(string query_domain, string rr_type, string rr_class) {
    //vector<Domain*> rrs(0);
    for (auto domain: authorized) {
        if (domain == query_domain) {
            query_domain = "@." + query_domain;
        }
        if (query_domain.find(domain) != string::npos) {
            cout << "yes\n";
            for (auto d: ans_table[domain]) {
                if (query_domain == d->domain_name + '.' + domain && rr_type == d->type && rr_class == d->class_) return d;
            }
        }
    }
    return NULL;
    // return rrs;
}

string domain_authorized(string query_domain) {
    for (auto domain: authorized) {
        if (query_domain.find(domain) != string::npos) return domain;
    }
    return "";
}

vector<string> split_by(string data, char by) {
    vector<string> res;
    string tmp = "";
    for (int i = 0; i < (int)data.length(); i++) {
        if (data[i] == by) {
            res.push_back(tmp);
            tmp = "";
        }
        else tmp += data[i];
    }
    if (tmp != "") res.push_back(tmp);
    return res;
}

bool check_nip(string domain) {
    vector<string> split = split_by(domain, '.');
    for (int i = 0; i < 4; i++) {
        for (auto c: split[i]) if (!(c >= '0' && c <= '9')) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    int MAXSIZE = 1000;
    int sockfd, PORT = -1;
    char buf[MAXSIZE];
    char* config_file;
    if (argc > 2) {
        PORT = atol(argv[1]);
        config_file = argv[2];
    }
    /*printf)"%s\n", config_file);*/
    // load config
    FILE* fptr = fopen(config_file, "r");
    string forwarder;
    // ans_table[zone][domain]
    fgets(buf, MAXSIZE, fptr);
    forwarder = buf;
    char* line = NULL;
    size_t len = MAXSIZE;
    while (getline(&line, &len, fptr) > 0) {
        //printf("reading: %s", line);
        int cur = 0;
        char* zone_config = (char*)malloc(sizeof(char) * 100);
        string zone_name = to_comma(line, cur);
        int i = 0;
        while (line[cur] && line[cur] != '\n' && line[cur] != '\r') {
            zone_config[i] = line[cur];
            cur++;
            i++;
        }
        authorized.push_back(zone_name);
        // parse zone config
        printf("reading: %s\n", zone_config);
        FILE* zone_fptr = fopen(zone_config, "r");
        char* line2 = NULL;
        size_t len2 = MAXSIZE;
        getline(&line2, &len2, zone_fptr);
        while (getline(&line2, &len2, zone_fptr) > 0) {
            int cur = 0;
            Domain* domain_data = (Domain*)malloc(sizeof(Domain));
            string domain_name = to_comma(line2, cur);
            string ttl = to_comma(line2, cur);
            string class_ = to_comma(line2, cur);
            string type = to_comma(line2, cur);
            string data = to_comma(line2, cur);
            domain_data->domain_name = domain_name;
            domain_data->ttl = ttl;
            domain_data->class_ = class_;
            domain_data->type = type;
            domain_data->data = data;
            ans_table[zone_name].push_back(domain_data);
        }
    }
    sort(authorized.begin(), authorized.end(), cmp);
    // socket
    struct sockaddr_in servaddr, cliaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Socket creation failed.\n");
        exit(1);
    }
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT);
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Bind failed.\n");
        exit(1);
    }
    int n;
    socklen_t clilen = sizeof(cliaddr);
    cout << "Starting service...\n";
    while (1) {
        memset(buf, 0, sizeof(buf));
        // receive
        if ((n = recvfrom(sockfd, (char*)buf, MAXSIZE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &clilen)) < 0) continue;
        //printf("%s\n", buf);
        //for (int i = 0; i < MAXSIZE; i++) printf("%03d %3d %c\n", i, buf[i], buf[i]);
        char* packet = (char*)malloc(sizeof(char) * MAXSIZE);
        memcpy(packet, buf, MAXSIZE);
        Header* header = (Header*)malloc(sizeof(Header));
        QS* qs = (QS*)malloc(sizeof(QS));
        char QNAME[100] = {0};
        memcpy(header, packet, sizeof(Header));
        packet += sizeof(Header);
        strcpy(QNAME, packet);
        packet += strlen(QNAME) + 1;
        memcpy(qs, packet, sizeof(QS));
        header->ID = convert_endian(header->ID);
        header->QDCOUNT = convert_endian(header->QDCOUNT);
        header->ANCOUNT = convert_endian(header->ANCOUNT);
        header->NSCOUNT = convert_endian(header->NSCOUNT);
        header->ARCOUNT = convert_endian(header->ARCOUNT);
        qs->QTYPE = convert_endian(qs->QTYPE);
        qs->QCLASS = convert_endian(qs->QCLASS);
        // process
        string query_domain = QNAME + 1;
        for (int i = 0; i < (int)query_domain.length(); i++) {
            if (query_domain[i] < 32 || query_domain[i] > 126) query_domain[i] = '.';
        }
        query_domain += '.';
        string rr_type = TYPE_INT_TO_STR[qs->QTYPE];
        string rr_class = CLASS_INT_TO_STR[qs->QCLASS];
        string authed = domain_authorized(query_domain);
        Domain* rr = find_rr(query_domain, rr_type, rr_class);
        bool found;
        if (rr || check_nip(query_domain)) found = 1;
        else found = 0;
        // reply
        if (authed != "") {
            memset(buf, 0, sizeof(buf));
            Header* res_header = (Header*)malloc(sizeof(Header));
            res_header->ID = convert_endian(header->ID);
            res_header->QR = 1;
            res_header->OPCODE = header->OPCODE;
            res_header->AA = 1; // under my domain
            res_header->TC = 0;
            res_header->RD = 1;
            res_header->RA = 1;
            res_header->Z = 0;
            res_header->RCODE = 0;
            res_header->QDCOUNT = convert_endian(header->QDCOUNT);
            res_header->ANCOUNT = convert_endian((found ? 1 : 0));
            res_header->NSCOUNT = convert_endian(1); //
            res_header->ARCOUNT = convert_endian(0); //
            if (rr_type == "SOA") {
                res_header->ANCOUNT = convert_endian(1);
                res_header->NSCOUNT = convert_endian(0);
            }
            int cur = 0;
            // append header
            memcpy(buf, res_header, sizeof(Header));
            cur += sizeof(Header);
            // append question section
            qs->QTYPE = convert_endian(qs->QTYPE);
            qs->QCLASS = convert_endian(qs->QCLASS);
            memcpy(buf + cur, QNAME, strlen(QNAME) + 1);
            cur += strlen(QNAME) + 1;
            memcpy(buf + cur, qs, sizeof(QS));
            cur += sizeof(QS);
            if (found) {
                // append answer section
                // NAME
                memcpy(buf + cur, QNAME, strlen(QNAME) + 1);
                cur += strlen(QNAME) + 1;
                // other
                uint32_t res;
                char res_aaaa[16];
                RR* rr_res = (RR*)malloc(sizeof(RR));
                rr_res->TYPE = qs->QTYPE;
                rr_res->CLASS = qs->QCLASS;
                rr_res->TTL = convert_endian32((rr ? stoi(rr->ttl) : 1000));
                if (rr_type == "A") {
                    if (check_nip(query_domain)) {
                        vector<string> split = split_by(query_domain, '.');
                        string tmp = "";
                        for (int i = 0; i < 4; i++) {
                            tmp += split[i];
                            if (i != 3) tmp += '.';
                        }
                        res = inet_addr(tmp.c_str());
                    }
                    else {
                        res = inet_addr(rr->data.c_str());
                    }
                    rr_res->RDLENGTH = sizeof(res);
                }
                else if (rr_type == "AAAA") {
                    rr_res->RDLENGTH = sizeof(res_aaaa);
                    inet_pton(AF_INET6, rr->data.c_str(), res_aaaa);
                }
                else if (rr_type == "TXT" || rr_type == "NS") rr_res->RDLENGTH = strlen(rr->data.c_str()) + 1;
                else if (rr_type != "SOA") rr_res->RDLENGTH = strlen(rr->data.c_str()) + 1;
                rr_res->RDLENGTH = convert_endian(rr_res->RDLENGTH);
                memcpy(buf + cur, rr_res, sizeof(RR));
                cur += sizeof(RR);
                // RDATA
                if (rr_type == "A") {
                    memcpy(buf + cur, &res, sizeof(res));
                    cur += sizeof(res);
                }
                else if (rr_type == "AAAA") {
                    memcpy(buf + cur, res_aaaa, sizeof(res_aaaa));
                    cur += sizeof(res_aaaa);
                }
                else {
                    if (rr_type == "MX") {
                        // parse
                        char rdata[100] = {0};
                        uint16_t preference = 0;
                        int k = 0;
                        for (k = 0; k < (int)strlen(rr->data.c_str()); k++) {
                            if (rr->data.c_str()[k] == ' ') break;
                            preference *= 10;
                            preference += rr->data.c_str()[k] - '0';
                        }
                        rdata[0] = 4;
                        for (int i = k + 1; i < (int)strlen(rr->data.c_str()); i++) rdata[i - k] = rr->data.c_str()[i];
                        int idx = 0;
                        int cnt = -1;
                        for (int i = 0; i < (int)strlen(rdata); i++) {
                            if (rdata[i] == '.') {
                                rdata[idx] = cnt;
                                idx = i;
                                cnt = 0;
                            }
                            else cnt++;
                        }
                        //rdata[idx] = cnt;
                        preference = convert_endian(preference);
                        memcpy(buf + cur, &preference, sizeof(preference));
                        cur += sizeof(preference);
                        rdata[strlen(rdata) - 1] = 0;
                        memcpy(buf + cur, rdata, strlen(rdata) + 1);
                        cur += strlen(rdata) + 1;
                    }
                    else if (rr_type == "NS") {
                        char rdata[100] = {0};
                        for (int i = 0; i < (int)strlen(rr->data.c_str()); i++) rdata[i + 1] = rr->data.c_str()[i];
                        int idx = 0;
                        int cnt = 0;
                        for (int i = 1; i <= (int)strlen(rdata + 1) + 1; i++) {
                            if (rdata[i] == '.') {
                                rdata[idx] = cnt;
                                idx = i;
                                cnt = 0;
                            }
                            else cnt++;
                        }
                        rdata[strlen(rdata) - 1] = 0;
                        memcpy(buf + cur, rdata, strlen(rdata) + 1);
                        cur += strlen(rdata) + 1;
                    }
                    else if (rr_type == "TXT") {
                        uint8_t L = strlen(rr->data.c_str());
                        memcpy(buf + cur, &L, sizeof(L));
                        cur += sizeof(L);
                        memcpy(buf + cur, rr->data.c_str(), strlen(rr->data.c_str()) + 1);
                        cur += strlen(rr->data.c_str()) + 1;
                    }
                    else if (rr_type != "SOA") {
                        //cout << rr->data << '\n';
                        char rdata[100] = {0};
                        for (int i = 0; i < (int)strlen(rr->data.c_str()); i++) rdata[i + 1] = rr->data.c_str()[i];
                        int idx = 0;
                        int cnt = 0;
                        for (int i = 1; i <= (int)strlen(rdata + 1) + 1; i++) {
                            if (rdata[i] == '.') {
                                rdata[idx] = cnt;
                                idx = i;
                                cnt = 0;
                            }
                            else cnt++;
                        }
                        rdata[strlen(rdata) - 1] = 0;
                        memcpy(buf + cur, rdata, strlen(rdata) + 1);
                        cur += strlen(rdata) + 1;
                    }
                }
            }
            // authorized section
            char* auth_name;
            if (check_nip(query_domain)) {
                auth_name = QNAME;
                for (int i = 0; i < 4; i++) {
                    auth_name += auth_name[0] + 1;
                }
            }
            else {
                if (QNAME[0] != 5) auth_name = QNAME + QNAME[0] + 1;
                else auth_name = QNAME;
            }
            memcpy(buf + cur, auth_name, strlen(auth_name) + 1);
            cur += strlen(auth_name) + 1;
            if (rr_type != "SOA") {
                if (found) {
                    // NS
                    Domain* ns_rr = find_rr(authed, "NS", "IN");
                    RR* rr_res_auth = (RR*)malloc(sizeof(RR));
                    rr_res_auth->TYPE = convert_endian(TYPE_STR_TO_INT["NS"]);
                    rr_res_auth->CLASS = convert_endian(CLASS_STR_TO_INT["IN"]);
                    rr_res_auth->TTL = convert_endian32(stoi(ns_rr->ttl));
                    rr_res_auth->RDLENGTH = convert_endian(strlen(ns_rr->data.c_str()) + 1);
                    memcpy(buf + cur, rr_res_auth, sizeof(RR));
                    cur += sizeof(RR);
                    char rdata2[100] = {0};
                    for (int i = 0; i < (int)strlen(ns_rr->data.c_str()); i++) rdata2[i + 1] = ns_rr->data.c_str()[i];
                    int idx = 0;
                    int cnt = 0;
                    for (int i = 1; i <= (int)strlen(rdata2 + 1) + 1; i++) {
                        if (rdata2[i] == '.') {
                            rdata2[idx] = cnt;
                            idx = i;
                            cnt = 0;
                        }
                        else cnt++;
                    }
                    rdata2[strlen(rdata2) - 1] = 0;
                    memcpy(buf + cur, rdata2, strlen(rdata2) + 1);
                    cur += strlen(rdata2) + 1;
                }
                else {
                    // SOA
                    Domain* soa_rr = find_rr(authed, "SOA", "IN");
                    vector<string> soa_data = split_by(soa_rr->data, ' ');
                    RR* rr_res_auth = (RR*)malloc(sizeof(RR));
                    rr_res_auth->TYPE = convert_endian(TYPE_STR_TO_INT["SOA"]);
                    rr_res_auth->CLASS = convert_endian(CLASS_STR_TO_INT["IN"]);
                    rr_res_auth->TTL = convert_endian32(stoi(soa_rr->ttl));
                    char rdata2[100] = {0};
                    int cur2 = 0;
                    // MNAME
                    char MNAME[100] = {0};
                    for (int i = 0; i < (int)soa_data[0].length(); i++) MNAME[i + 1] = soa_data[0][i];
                    int idx = 0;
                    int cnt = 0;
                    for (int i = 1; i <= (int)strlen(MNAME + 1) + 1; i++) {
                        if (MNAME[i] == '.') {
                            MNAME[idx] = cnt;
                            idx = i;
                            cnt = 0;
                        }
                        else cnt++;
                    }
                    MNAME[strlen(MNAME) - 1] = 0;
                    memcpy(rdata2 + cur2, MNAME, strlen(MNAME) + 1);
                    cur2 += strlen(MNAME) + 1;
                    // RNAME
                    char RNAME[100] = {0};
                    for (int i = 0; i < (int)soa_data[1].length(); i++) RNAME[i + 1] = soa_data[1][i];
                    idx = 0;
                    cnt = 0;
                    for (int i = 1; i <= (int)strlen(RNAME + 1) + 1; i++) {
                        if (RNAME[i] == '.') {
                            RNAME[idx] = cnt;
                            idx = i;
                            cnt = 0;
                        }
                        else cnt++;
                    }
                    RNAME[strlen(RNAME) - 1] = 0;
                    memcpy(rdata2 + cur2, RNAME, strlen(RNAME) + 1);
                    cur2 += strlen(RNAME) + 1;
                    // SERIAL
                    uint32_t SERIAL = convert_endian32(stoi(soa_data[2]));
                    memcpy(rdata2 + cur2, &SERIAL, sizeof(SERIAL));
                    cur2 += sizeof(SERIAL);
                    // REFRESH
                    uint32_t REFRESH = convert_endian32(stoi(soa_data[3]));
                    memcpy(rdata2 + cur2, &REFRESH, sizeof(REFRESH));
                    cur2 += sizeof(REFRESH);
                    // RETRY
                    uint32_t RETRY = convert_endian32(stoi(soa_data[4]));
                    memcpy(rdata2 + cur2, &RETRY, sizeof(RETRY));
                    cur2 += sizeof(RETRY);
                    // EXPIRE
                    uint32_t EXPIRE = convert_endian32(stoi(soa_data[5]));
                    memcpy(rdata2 + cur2, &EXPIRE, sizeof(EXPIRE));
                    cur2 += sizeof(EXPIRE);
                    // MINIMUM
                    uint32_t MINIMUM = convert_endian32(stoi(soa_data[6]));
                    memcpy(rdata2 + cur2, &MINIMUM, sizeof(MINIMUM));
                    cur2 += sizeof(MINIMUM);
                    // append
                    int size = 5 * sizeof(uint32_t) + strlen(RNAME) + 1 + strlen(MNAME) + 1;
                    rr_res_auth->RDLENGTH = convert_endian(size);
                    memcpy(buf + cur, rr_res_auth, sizeof(RR));
                    cur += sizeof(RR);
                    memcpy(buf + cur, rdata2, size);
                    cur += size;
                }
            }
            sendto(sockfd, buf, cur, 0, (struct sockaddr*)&cliaddr, clilen);
        }
        else {
            int sock_forwarder;
            struct sockaddr_in forwarder_addr;
            char forwarder_char[MAXSIZE] = {0};
            for (int i = 0; i < (int)forwarder.length(); i++) forwarder_char[i] = forwarder[i];
            forwarder_char[strlen(forwarder_char) - 1] = 0;
            sock_forwarder = socket(AF_INET, SOCK_DGRAM, 0);
            memset(&forwarder_addr, 0, sizeof(forwarder_addr));
            forwarder_addr.sin_family = AF_INET;
            forwarder_addr.sin_addr.s_addr = inet_addr(forwarder_char);
            forwarder_addr.sin_port = htons(53);
            socklen_t forwarder_len = sizeof(forwarder_addr);
            char buf2[MAXSIZE];
            if (sendto(sock_forwarder, buf, MAXSIZE, 0, (struct sockaddr*) &forwarder_addr, forwarder_len) < 0) {
                printf("Send to forwarder error\n");
                continue;
            }
            if ((recvfrom(sock_forwarder, (char*)buf2, MAXSIZE, MSG_WAITALL, (struct sockaddr*) &forwarder_addr, &forwarder_len)) < 0) printf("error\n");
            sendto(sockfd, buf2, sizeof(buf2), 0, (struct sockaddr*) &cliaddr, clilen);
        }
    }
    return 0;
}

