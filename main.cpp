#include <iostream>
#include <omp.h>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

inline unsigned char get_mn(const long *r, long need) {
    long c = 0;
    for (int i = 0; i < 256; i++) {
        c += r[i];
        if (c > need) {
            return i;
        }
    }
    return 255;
}

inline unsigned char get_mx(const long *r, long need) {
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
    return (unsigned char) round(x);
}

int main(int argc, char *argv[]) {

    if (argc < 5) {
        cout << "invalid arguments" << endl;
        return 0;
    }

    int thr;
    double threshold;

    try {
        thr = stoi(argv[1]);
        threshold = stof(argv[4]);
    } catch (const invalid_argument &e) {
        cout << "Error in " << e.what() << endl << "It is not a number" << endl;
        return 0;
    }

    thr = thr == 0 ? omp_get_max_threads() : thr;
    omp_set_num_threads(thr);

    FILE *input = fopen(argv[2], "rb");
    if (!input) {
        cout << "Wrong file name" << endl;
        return 0;
    }
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);
    auto *arr = new unsigned char[size];
    if (size != fread(arr, sizeof(unsigned char), size, input)) {
        cout << "Something went wrong while read, try again." << endl;
        return 0;
    }
    fclose(input);

    if (arr[0] != 'P' || (arr[1] != '5' && arr[1] != '6')) {
        cout << "It is not portable bitmap image file" << endl;
        return 0;
    }
    bool rgb = arr[1] == '6';
    long w = 0, h = 0, pos = 2;
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

    double start = omp_get_wtime();

    long r[256] = {0};
    long g[256] = {0};
    long b[256] = {0};

    int step = rgb ? 3 : 1;

    long block = size / thr;
    while (rgb && block % 3) block++;
#pragma omp parallel for shared(arr, block, pos, size, rgb, step, r, g, b, cout) default(none) schedule(dynamic)
    for (long k = pos; k < size; k += block) {
        long rc[256] = {0}, gc[256] = {0}, bc[256] = {0};
        long end = min((long) size, k + block);
        for (long i = k; i < end; i += step) {
            rc[arr[i]]++;
            if (rgb) {
                gc[arr[i + 1]]++;
                bc[arr[i + 2]]++;
            }
        }
#pragma omp critical
        {
            for (int i = 0; i < 256; i++) {
                r[i] += rc[i];
                g[i] += gc[i];
                b[i] += bc[i];
            }
        }
    }

    long need = (long) (w * h * threshold);
    unsigned char mn = rgb ? min({get_mn(r, need), get_mn(g, need), get_mn(b, need)}) : get_mn(r, need);
    unsigned char mx = rgb ? max({get_mx(r, need), get_mx(g, need), get_mx(b, need)}) : get_mx(r, need);

    cout << (int) mn << ' ' << (int) mx << endl;
    unsigned char values[256];
    for (int i = 0; i < 256; i++) {
        values[i] = overflow(255.0 * ((double) (i - mn) / (double) (mx - mn)));
    }

    if (mn != mx) {
#pragma omp parallel for shared(arr, pos, size, values) default(none) schedule(dynamic, 65536)
        for (long i = pos; i < size; i++) {
            arr[i] = values[arr[i]];
        }
    }

    cout << "Time (" << thr << " thread(s)): " << 1000 * (omp_get_wtime() - start) << " ms" << endl;
    FILE *output = fopen(argv[3], "wb");
    fwrite(arr, sizeof(unsigned char), size, output);
    fclose(output);
    delete[] arr;
}