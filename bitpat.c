#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

bool bitpat_match_s(uint16_t t, const char* s){
    int idx = 0;
    int bit_cnt = 0;

    if(s[idx++] != '0'){
        printf("Invalid pattern\n");
        return false;
    }
    if(s[idx++] != 'b'){
        printf("Invalid pattern\n");
        return false;
    }

    while(bit_cnt < 16){
        if(s[idx] == '_') {
            idx++;
            continue;
        }
        if(s[idx] == 'x'){
            idx++;
            bit_cnt++;
            continue;
        }
        if(s[idx] == '1'){
            if((t>>(15-bit_cnt))&1) {
                idx++;
                bit_cnt++;
                continue;
            }
            return false;
        }
        else if(s[idx] == '0'){
            if(((t>>(15-bit_cnt))&1) == 0) {
                idx++;
                bit_cnt++;
                continue;
            }
            return false;
        }
    }
    return true;
}

int main(){
    uint16_t test = 0b1010101010101010;
    char *pattern = "0bx010_1010_1010_1010";
    if(bitpat_match_s(test, pattern)){
        printf("true\n");
    }else{
        printf("false\n");
    }
}
