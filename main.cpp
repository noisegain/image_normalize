#include <iostream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include "chrono"

using namespace std;

inline unsigned char get_mn(const long *r, int need) {
    long c = 0;
    for (int i = 0; i < 256; i++) {
        c += r[i];
        if (c > need) {
            return i;
        }
    }
    return 255;
}

inline unsigned char get_mx(const long *r, int need) {
    long c = 0;
    for (int i = 255; i >= 0; i--) {
        c += r[i];
        if (c > need) {
            return i;
        }
    }
    return 0;
}

inline unsigned char overflow(double x) {
    if (x > 255) return 255;
    if (x < 0) return 0;
    return (int) x;
}



int main() {
    omp_set_num_threads(8);
    const char *name = "/mnt/c/Users/ispon/CLionProjects/skakov/test/baboon_copy.pgm";
    FILE* input = fopen(name, "rb");
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    cout << size << endl;
    fseek(input, 0, SEEK_SET);
    auto *arr = new unsigned char[size];
    if (size != fread(arr, sizeof(unsigned char), size, input)) {
        cout << "something went wrong while read, try again." << endl;
        return 1337;
    }
    fclose(input);
    auto start = chrono::high_resolution_clock::now();
    if (arr[0] != 'P' || (arr[1] != '5' && arr[1] != '6')) {
        cout << "Broken file" << endl;
        return 228;
    }
    int w = 0, h = 0, pos = 2;
    while (isspace(arr[pos])) pos++;
    pos--;
    while (!isspace(arr[++pos])) {
        w *= 10;
        w += arr[pos] - '0';
    }
    while (!isspace(arr[++pos])) {
        h *= 10;
        h += arr[pos] - '0';
    }
    while (isspace(arr[pos++]));
    pos += 3;
    while (isspace(arr[pos])) pos++;
    cout << w << " " << h << endl;

    long r[256]={0};
    long g[256]={0};
    long b[256]={0};

    auto start2 = chrono::high_resolution_clock::now();

#pragma omp parallel sections shared(arr, pos, size, r, g, b) default(none)
    {
#pragma omp section
        for (int i = pos; i < size; i += 3) {
            r[arr[i]]++;
        }
#pragma omp section
        for (int i = pos + 1; i < size; i += 3) {
            g[arr[i]]++;
        }
#pragma omp section
        for (int i = pos + 2; i < size; i += 3) {
            b[arr[i]]++;
        }
    }



//    for (int i = pos; i < size; i += 3) {
//        r[arr[i]]++;
//        g[arr[i + 1]]++;
//        b[arr[i + 2]]++;
//    }

    auto end1 = chrono::high_resolution_clock::now();
    cout << "This: " << chrono::duration_cast<chrono::milliseconds>(end1 - start2).count() << "ms" << endl;

    //unsigned char mn = *min_element(arr + pos, arr + size);
    //unsigned char mx = *max_element(arr + pos, arr + size);

    double threshold = 0.05;
    int need = w * h * threshold;
    unsigned char mn = min({ get_mn(r, need), get_mn(g, need), get_mn(b, need) });
    unsigned char mx = max({ get_mx(r, need), get_mx(g, need), get_mx(b, need) });

//    #pragma omp parallel for shared(arr, pos, block, size, mn, mx, cout) default(none) schedule(static, 100)
//    for (int i = pos; i < size; i += block) {
//        unsigned char curmn = 255;
//        unsigned char curmx = 0;
//        int end = min((int) size, i + block);
//        for (int j = i; j < end; j++) {
//            curmn = min(curmn, arr[j]);
//            curmx = max(curmx, arr[j]);
//        }
//#pragma omp critical
//        {
//            mn = min(mn, curmn);
//            mx = max(mx, curmx);
//        }
//    }
    cout << (int) mn << " " << (int) mx << endl;
    cout << size - pos << endl;
#pragma omp parallel for shared(arr, pos, size, mn, mx) default(none) //schedule(static, 1000)
    for (int i = pos; i < size; i++) {
        arr[i] = overflow(255.0 * ((double) (arr[i] - mn) / (double) (mx - mn)));
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Time: " << duration.count() << " ms" << endl;
    FILE* output = fopen("/mnt/c/Users/ispon/CLionProjects/skakov/test/out.pgm", "wb");
    fwrite(arr, sizeof(unsigned char), size, output);
    fclose(output);
}
