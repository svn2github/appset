#include <apknixengine.h>
#include <iostream>

using namespace APK;
using namespace std;

class Listener:public EngineListener{
public:
    void step(const char *content){
        cout  << content << endl;
        cout.flush();
    }
};

int main(int argc, const char *argv[]){
    NIXEngine apk;
    Listener list;

    if(argc<2) return 1;

    cout << "CONFIGURE: " << apk.configure("/home/simone/apk.conf") << endl;

    Package p;
    p.setName(argv[1]);

    apk.addListener(new Listener());

    apk.install(&p);

    return 0;
}
