#include<iostream>
#include<time.h>
#include<cstdlib>

using namespace std;

int main() {
    const int max = 5;
    int rds[max];
    for (int i = 0; i < max; i++) cin >> rds[i];
    int x = time(NULL);
    int right = 0;
    while (1) {
        srand(x);
        for (int i = 0; i < max; i++) {
            if (rds[i] != rand()) break;
            right++;
        }
        if (right == max) break;
        else x--;
    }
    cout << x << '\n';
    cout << rand() << '\n';
    return 0;
}


