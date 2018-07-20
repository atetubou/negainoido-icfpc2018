#include <iostream>
#include <fstream>
#include <vector>

using namespace std;
using v = vector<int>;
using vv = vector<v>;
using vvv = vector<vv>;

vvv
read_mdl(char*fn) {
    ifstream is (fn);
    char *buffer = new char[1];
    is.read(buffer, 1);
    int R = (unsigned int)buffer[0];
    vvv M(R, vv(R, v(R, 0)));
    int i = 8;
    for (int x=0; x<R; ++x) {
        for (int y=0; y<R; ++y) {
            for (int z=0; z<R; ++z) {
                if (i >= 8) {
                    is.read(buffer, 1);
                    i = 0;
                }
                if (buffer[0] & (1 << i)) {
                    M[x][y][z] = 1;
                }
                i += 1;
            }
        }
    }
    return M;
}

main(int argc, char*argv[]) {

    vvv M = read_mdl(argv[1]);
    int R = M.size();
    cout << R << endl;

    for (int x=0;x<R;++x) {
        for (int y=0; y<R; ++y) {
            for (int z=0;z<R;++z) {
                cout << M[x][y][z];
            }
            cout << endl;
        }
        cout << endl;
    }

}
