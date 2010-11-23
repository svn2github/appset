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

int main(){
    NIXEngine apk;
    Listener list;

    cout << "CONFIGURE: " << apk.configure("/home/simone/apk.conf") << endl;

    apk.addListener(&list);

    cout << "UPDATE: " << apk.update() << endl;

    apk.removeListener(&list);

    cout << "QUERYING UPGRADABLE PACKETS..." << endl;

    std::list<Package*>* pkgs = apk.queryLocal(APK_QUERY_UPGRADABLE);

    for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
        std::list<Package*>* pkg= apk.queryLocal(APK_QUERY_ALL_INFO, (*it));
        std::list<Package*>* rem= apk.queryRemote(APK_QUERY_BY_NAME, (*it));
        cout << endl << ":: " << ((*it)->getName())  << " ( " << ((*pkg->begin())->getLocalVersion());
        cout << " ---> " << ((*rem->begin())->getRemoteVersion()) << ")";
        cout << endl << "::\t" << ((*pkg->begin())->getDescription()) << endl;
        delete (*pkg->begin());
        delete (pkg);
        delete (*rem->begin());
        delete (rem);
    }

    if(pkgs->size()){
        cout << endl << "DO YOU WANT TO UPGRADE THIS ONES? [y/n] ";
        cout.flush();
        char c;
        cin >> c;

        if(c=='y'){
            apk.addListener(&list);
            c=apk.upgrade();
            apk.removeListener(&list);
            if(!c){
                for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
                    std::list<Package*>* pkg= apk.queryLocal(APK_QUERY_ALL_INFO, (*it));
                    cout << "UPGRADED PACKAGE: " << ((*it)->getName())  << " ( Version: " << ((*pkg->begin())->getLocalVersion());
                    cout << ", Description: " << ((*pkg->begin())->getDescription()) << " )" << endl;
                    delete (*it);
                    delete (*pkg->begin());
                    delete (pkg);
                }
            }
        }
    }else cout << "NO UPGRADABLE PACKAGES..." << endl;

    delete pkgs;

    return 0;
}
