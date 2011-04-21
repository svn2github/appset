/*
Copyright 2010 Simone Tobia

This file is part of AppSet.

AppSet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppSet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppSet; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "asnixengine.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/wait.h>
#include <regex.h>

using namespace std;

AS::NIXEngine::NIXEngine(){
    sysinfo.insert(StrPair("distribution",""));
    sysinfo.insert(StrPair("tool",""));
    sysinfo.insert(StrPair("as_conf_path",""));

    commands.insert(StrPair("update",""));
    commands.insert(StrPair("upgrade",""));
    commands.insert(StrPair("install",""));
    commands.insert(StrPair("remove",""));
    commands.insert(StrPair("query_upgradable",""));
    commands.insert(StrPair("query_local_byname",""));
    commands.insert(StrPair("query_local_info_byname",""));
    commands.insert(StrPair("query_remote_byname",""));
    commands.insert(StrPair("query_remote_info_byname",""));
    commands.insert(StrPair("tool_ignore_upgrades",""));
    commands.insert(StrPair("tool_hold_upgrades",""));
    commands.insert(StrPair("tool_unhold_upgrades",""));
    commands.insert(StrPair("check_install_deps",""));
    commands.insert(StrPair("check_remove_deps",""));
    commands.insert(StrPair("check_upgrade_deps",""));
    commands.insert(StrPair("download_path",""));
    commands.insert(StrPair("tool_lock_file",""));
    commands.insert(StrPair("clean_cache",""));
    commands.insert(StrPair("tool_post_up_cmd",""));

    commands.insert(StrPair("install_local",""));
    commands.insert(StrPair("local_ext",""));
    commands.insert(StrPair("check_local_deps",""));

    regexps.insert(StrPair("query_filter_regexp",""));
    regexps.insert(StrPair("query_repo_sep",""));
    regexps.insert(StrPair("query_name_regexp",""));
    regexps.insert(StrPair("query_lversion_regexp",""));
    regexps.insert(StrPair("query_info_name_regexp",""));
    regexps.insert(StrPair("query_info_version_regexp",""));
    regexps.insert(StrPair("query_info_group_regexp",""));
    regexps.insert(StrPair("query_info_license_regexp",""));
    regexps.insert(StrPair("query_info_description_regexp",""));
    regexps.insert(StrPair("query_info_url_regexp",""));
    regexps.insert(StrPair("query_info_ksize_regexp",""));
    regexps.insert(StrPair("query_info_repo_regexp",""));

    commands.insert(StrPair("community_tool",""));

    community.insert(StrPair("community_name",""));
    community.insert(StrPair("install",""));
    community.insert(StrPair("remove",""));
    community.insert(StrPair("upgrade",""));
    community.insert(StrPair("search",""));
    community.insert(StrPair("query_info",""));
    //community.insert(StrPair("cmd_exec",""));

    community_enabled=false;
}

AS::NIXEngine::~NIXEngine(){
    unlink(pipePath.c_str());
}

int AS::NIXEngine::loadConfigFile(const char *path, StrMap *params, int fErrorRet, int paramErrorRet){
    ifstream confFile;
    string line,key,value;
    int eqIndex;
    StrMap::iterator it;

    confFile.open(path);

    if(!confFile.is_open()) return fErrorRet;

    while(getline(confFile,line)){
        if( ( eqIndex = line.find('=') ) == string::npos ) continue;

        key = line.substr(0, eqIndex);
        value = line.substr(eqIndex+1);

        /*if( ( it = params->find(key) ) != params->end() )
            it->second = value;*/
        (*params)[key]=value;
    }

    confFile.close();

    for( it=params->begin(); it!=params->end(); it++){
        if(it->second.empty()) return paramErrorRet;
    }

    return 0;
}

int AS::NIXEngine::execCmd(string command){
    string buffer;
    string env = "LANG=en ";
    command += " > ";
    command += pipePath;
    command += " 2> ";
    command += pipePath;
    env+=command;
    int status = 0;
    ifstream output;

    pid_t pid = fork();
    if(pid == 0){
        ::exit(system(env.c_str()));
    }else if(pid == -1){
        return 1;
    }else{
        output.open(pipePath.c_str());
        while(getline(output,buffer).good()){
            notifyListeners(buffer.c_str());
        }
        waitpid(pid,&status,0);
        if(output.is_open()) output.close();
    }

    return status;
}

int AS::NIXEngine::configure(const char *confFilePath, const char *pipePath, bool force){
    string path,com_path,tool_check;
    int ret = 0;

    if( ( ret = loadConfigFile(confFilePath, &sysinfo) ) ) return ret;

    path = sysinfo["as_conf_path"];
    path.append(sysinfo["distribution"]);
    path.append("/");
    com_path=path;
    path.append(sysinfo["tool"]);

    if( ( ret = loadConfigFile(path.c_str(), &commands, 3, 4) ) ) return ret;

    if( ( ret = loadConfigFile(path.c_str(), &regexps, 5, 6) ) ) return ret;

    if( getuid() && !force) return 7;

    if( mkfifo(pipePath, 0700) ) return 8;

    this->pipePath = string(pipePath);

    com_path.append(commands["community_tool"]);
    tool_check=string("which ");
    tool_check.append(commands["community_tool"]);
    tool_check.append(" >/dev/null 2>/dev/null");
    bool community_tool=commands["community_tool"].find('*')==std::string::npos;
    community_enabled=community_tool && (loadConfigFile(com_path.c_str(), &community)==0) && (system(tool_check.c_str())==0);

    return 0;
}

int AS::NIXEngine::saveConfig(const char *distName, const char *toolName, const char *confWrapperPath, const char *confFilePath){
    distName=toolName=confWrapperPath=confFilePath=0;

    return configure(confFilePath);
}

int AS::NIXEngine::update(){
    return execCmd(commands["update"]);
}

int AS::NIXEngine::toolUpgrade(){
    return execCmd(commands["tool_post_up_cmd"]);
}

int AS::NIXEngine::upgrade(std::list<Package*>* ignore_packages){
    string cmd(commands["upgrade"]);
    string tail("");
    int status=0;
    bool holding=false;
    bool ignoring=false;

    if(ignore_packages && ignore_packages->size()){
        holding=commands["tool_hold_upgrades"].find('*')==std::string::npos;
        if(!holding){
            ignoring=commands["tool_ignore_upgrades"].find('*')==std::string::npos;
            if(ignoring){
                tail+=" ";
                tail+=commands["tool_ignore_upgrades"];
            }
        }

        tail+=" ";
        bool first=true;
        for(std::list<Package*>::iterator it=ignore_packages->begin();it!=ignore_packages->end();it++){
            if(!first){
                if(ignoring)tail += ",";
                else tail += " ";
            }
            tail += (*it)->getName();
            first=false;
        }

        if(holding){
            string lcmd=commands["tool_hold_upgrades"];
            lcmd+=tail;
            status+=execCmd(lcmd);
        }
    }

    if(!holding) cmd += tail;

    status += execCmd( cmd );

    if(holding){
        string lcmd=commands["tool_unhold_upgrades"];
        lcmd+=tail;
        status+=execCmd(lcmd);
    }

    return status;
}

int AS::NIXEngine::install(std::list<Package*>* packages, bool local){
    if(!packages) return 1;
    string cmd(local?commands["install_local"]:commands["install"]);

    for(std::list<Package*>::iterator it=packages->begin();it!=packages->end();it++){
        cmd += " ";
        cmd += (*it)->getName();
    }

    return execCmd(cmd);
}

int AS::NIXEngine::remove(std::list<Package*>* packages){
    if(!packages) return 1;
    string cmd(commands["remove"]);

    for(std::list<Package*>::iterator it=packages->begin();it!=packages->end();it++){
        cmd += " ";
        cmd += (*it)->getName();
    }

    return execCmd(cmd);
}

namespace AS {
    class QueryListener : public AS::EngineListener{
        regex_t filter;
        regex_t pkg_name;
        regex_t pkg_version;
        char sep;

        std::list<AS::Package*>* pkgList;

        bool remote;
    public:
        QueryListener(std::list<AS::Package*>* pkgList, const char *filter, const char *pkgNameFilter, const char *pkgVersionFilter, bool remote=false,char sep='/'){
            regcomp(&this->filter, filter, REG_EXTENDED);
            regcomp(&pkg_name, pkgNameFilter, REG_EXTENDED);
            regcomp(&pkg_version, pkgVersionFilter, REG_EXTENDED);

            this->pkgList = pkgList;

            this->remote = remote;

            this->sep=sep;
        }

        ~QueryListener(){
            regfree(&filter);
            regfree(&pkg_name);
            regfree(&pkg_version);
        }

        void step(const char *content){
            regmatch_t match;
            string cstr(content), pname, version, repo;
            AS::Package *pkg = new AS::Package(!remote);

            if(!regexec(&filter, content, 1, &match, 0)){
                if(!regexec(&pkg_name, content, 1, &match, 0)){
                    pname = cstr.substr(match.rm_so, match.rm_eo-match.rm_so);
                    repo=pname.substr(0,pname.find(sep));
                    pname=pname.substr(pname.find(sep)+1);

                    if(!regexec(&pkg_version, content, 1, &match, 0)){
                        version = cstr.substr(match.rm_so, match.rm_eo-match.rm_so);

                        pkg->setName(pname);
                        pkg->setRepository(repo);
                        if(remote) pkg->setRemoteVersion(version);
                        else pkg->setLocalVersion(version);

                        pkgList->insert(pkgList->end(), pkg);
                        return;
                    }
                }                
            }

            delete pkg;
        }
    };

    class QueryInfoListener : public AS::EngineListener{
        regex_t pkg_name;
        regex_t pkg_version;
        regex_t pkg_group;
        regex_t pkg_license;
        regex_t pkg_description;
        regex_t pkg_url;
        regex_t pkg_size;
        regex_t pkg_repo;

        regex_t lib_filter;

        std::list<AS::Package*>* pkgList;

        bool remote;

        bool discard;
        bool expert;

        bool firstName;
    public:
        QueryInfoListener(std::list<AS::Package*>* pkgList, const char *pkgNameFilter, const char *pkgVersionFilter, const char *pkgGroupFilter,
                          const char *pkgLicenseFilter, const char *pkgDescFilter, const char *pkgUrlFilter, const char *pkgSizeFilter,const char *pkgRepoFilter,bool remote=false, bool expert=false){
            regcomp(&pkg_name, pkgNameFilter, REG_EXTENDED);
            regcomp(&pkg_version, pkgVersionFilter, REG_EXTENDED);
            regcomp(&pkg_group, pkgGroupFilter, REG_EXTENDED);
            regcomp(&pkg_license, pkgLicenseFilter, REG_EXTENDED);
            regcomp(&pkg_description, pkgDescFilter, REG_EXTENDED);
            regcomp(&pkg_url, pkgUrlFilter, REG_EXTENDED);
            regcomp(&pkg_size, pkgSizeFilter, REG_EXTENDED);
            regcomp(&pkg_repo, pkgRepoFilter, REG_EXTENDED);
            regcomp(&lib_filter, "lib[s]*.*|.*lib[s]*|.*-lib[s]*.*|.*lib[s]*-.*|ttf-.*|.*-data", REG_EXTENDED);

            this->pkgList = pkgList;

            this->remote = remote;
            this->expert = expert;

            discard=false;

            firstName=false;
        }

        ~QueryInfoListener(){
            regfree(&pkg_name);
            regfree(&pkg_version);
            regfree(&pkg_group);
            regfree(&pkg_license);
            regfree(&pkg_description);
            regfree(&pkg_url);
            regfree(&pkg_size);
            regfree(&pkg_repo);
            regfree(&lib_filter);
        }

        void step(const char *content){
            regmatch_t match,aux;
            string cstr(content), value;

            if(!regexec(&pkg_repo, content, 1, &match, 0)){
                if(!expert && !regexec(&lib_filter, content, 1, &aux, 0)){
                    discard=true;
                    return;
                }

                discard=false;
                value = cstr.substr(match.rm_eo);

                if(!firstName){
                    AS::Package *pkg = new AS::Package(!remote);
                    pkg->setRepository(value);
                    pkgList->insert(pkgList->end(), pkg);
                }else{
                    (*(pkgList->rbegin()))->setRepository(value);
                }
            }else if(discard){
                return;
            }else if(!regexec(&pkg_name, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                if(!remote || pkgList->empty() || firstName){
                    AS::Package *pkg = new AS::Package(!remote);
                    pkg->setName(value);
                    pkgList->insert(pkgList->end(), pkg);
                    firstName=true;
                }else
                    (*(pkgList->rbegin()))->setName(value);
            }else if(!regexec(&pkg_version, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                if(remote)(*(pkgList->rbegin()))->setRemoteVersion(value);
                else (*(pkgList->rbegin()))->setLocalVersion(value);
            }else if(!regexec(&pkg_group, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                (*(pkgList->rbegin()))->setGroup(value);
            }else if(!regexec(&pkg_license, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                (*(pkgList->rbegin()))->setLicense(value);
            }else if(!regexec(&pkg_description, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                (*(pkgList->rbegin()))->setDescription(value);
            }else if(!regexec(&pkg_url, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);
                (*(pkgList->rbegin()))->setURL(value);
            }else if(!regexec(&pkg_size, content, 1, &match, 0)){
                value = cstr.substr(match.rm_eo);

                int ksize=-1;
                for(int j=0;ksize==-1 && j<((unsigned)value.length());++j){
                    if(*(value.c_str()+j)=='.'){
                        value=value.substr(0,j);
                        ksize=atoi(value.c_str());
                    }
                }

                (*(pkgList->rbegin()))->setSize(ksize);
            }
        }
    };
}

int AS::NIXEngine::execQuery(std::list<AS::Package*>* pkgList, unsigned flags, AS::Package *package, bool remote, bool local){
    int status=0;
    QueryListener ql(pkgList, regexps["query_filter_regexp"].c_str(), regexps["query_name_regexp"].c_str(), regexps["query_lversion_regexp"].c_str(), remote);
    addListener(&ql);

    //if(flags & as_QUERY_UPGRADABLE){
        //status = execCmd(commands["query_upgradable"]);
    //}else
    if(flags & as_QUERY_BY_NAME){
        string cmd = remote?commands["query_remote_byname"]:commands["query_local_byname"];

        if(package){
            cmd += " ";
            cmd += package->getName();
        }

        //if(flags & as_EXPERT_QUERY) cmd += " | grep -E -v -e \"lib[s]*.*|.*lib[s]*|.*-lib[s]*.*|.*lib[s]*-.*|ttf-.*\"";

        status = execCmd(cmd);
    }else if(flags & as_QUERY_DEPS){
        string cmd = local?commands["check_local_deps"]:remote?commands["check_install_deps"]:commands["check_remove_deps"];

        cmd += " ";
        cmd += package->getName();

        status = execCmd(cmd);
    }else if(flags & as_UPGRADE_DEPS){
        string cmd = commands["check_upgrade_deps"];

        cmd += " ";
        cmd += package->getName();

        status = execCmd(cmd);
    }

    removeListener(&ql);

    if((flags & as_QUERY_ALL_INFO) || ((flags & as_QUERY_UPGRADABLE))){
        QueryInfoListener qil(pkgList, regexps["query_info_name_regexp"].c_str(), regexps["query_info_version_regexp"].c_str(), regexps["query_info_group_regexp"].c_str(),
                              regexps["query_info_license_regexp"].c_str(), regexps["query_info_description_regexp"].c_str(), regexps["query_info_url_regexp"].c_str(),
                              regexps["query_info_ksize_regexp"].c_str(),regexps["query_info_repo_regexp"].c_str(), remote, flags & as_EXPERT_QUERY);

        addListener(&qil);

        if(flags & as_QUERY_UPGRADABLE)status = execCmd(commands["query_upgradable"]);
        else{
            string cmd = remote?commands["query_remote_info_byname"]:commands["query_local_info_byname"];

            if(package){
                cmd += " ";
                cmd += package->getName();
            }

            //cmd += " | grep -E -e \"Name|Description|Version|URL\"";
            //if(!()) cmd += " | grep -E -v -e \"lib[s]*.*|.*lib[s]*|.*-lib[s]*.*|.*lib[s]*-.*|multilib[s]*.*|.*multilib[s]*|.*-multilib[s]*.*|.*multilib[s]*-.*|ttf-.*\"";

            status = execCmd(cmd);
        }

        removeListener(&qil);
    }

    return status;
}

std::list<AS::Package*>* AS::NIXEngine::checkDeps(AS::Package *package, bool install, bool upgrade, bool local){
    if(!package) return 0;

    std::list<AS::Package*>* ret = new std::list<AS::Package*>();

    int status = upgrade?execQuery(ret, as_UPGRADE_DEPS, package, install):execQuery(ret, as_QUERY_DEPS, package, install, local);

    if(status){
        delete ret;
        ret = 0;
    }

    return ret;
}

std::list<AS::Package*>* AS::NIXEngine::queryLocal(unsigned flags, AS::Package *package){
    std::list<AS::Package*>* ret = new std::list<AS::Package*>();

    execQuery(ret, flags, package, false);

    return ret;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

std::list<AS::Package*>* AS::NIXEngine::queryRemote(unsigned flags, AS::Package *package){
    std::list<AS::Package*>* ret = new std::list<AS::Package*>();
    int status=0;


    if(flags & as_MERGE_QUERIES){
        const char *helperPath = "/tmp/ashelper.out";
        struct stat s;

        status = stat(helperPath,&s);

        string buffer;
        ifstream output;

        output.open(helperPath);
        int part=0;
        AS::Package *pkg;
        while(getline(output,buffer).good()){
            switch(part){
            case 0:
                pkg = new AS::Package();
                ret->insert(ret->end(),pkg);
                pkg->setName(buffer);
                break;
            case 1:
                pkg->setLocalVersion(buffer);
                break;
            case 2:
                pkg->setRemoteVersion(buffer);
                break;
            case 3:
                pkg->setSize(atoi(buffer.c_str()));
                break;
            case 4:
                pkg->setDescription(buffer);
                pkg->setQueried();
                break;
            case 5:
                pkg->setURL(buffer);;
                break;
            case 6:
                pkg->setInstalled(buffer.compare("false"));
                break;
            case 7:
                pkg->setRepository(buffer);
            }
            part = (part+1)%8;
        }
        if(output.is_open()) output.close();
    }else{
        status = execQuery(ret, flags, package, true);
    }

    if(status){
        delete ret;
        ret = 0;
    }

    return ret;
}

int AS::NIXEngine::getProgressSize(AS::Package *package, bool deps){
    if(!package) return 0;

    DIR *dpath = opendir(commands["download_path"].c_str());
    if(dpath==NULL) return 0;
    dirent *dir;
    std::string fname;
    int ret=0;

    bool found=false;
    while(!found && (dir=readdir(dpath))){
        fname=string(dir->d_name);

        std::string pname = package->getName();

        int pos=-1;
        if( ( pos=fname.find( (pname/*+="-"*/) ))!=std::string::npos &&
                (deps || fname.find(package->getRemoteVersion())!= std::string::npos) ){
            if(pos) continue;
            pos=pname.length()+1;
            if(!isdigit(fname.at(pos))) continue;
            std::string fpath=commands["download_path"];
            fpath+=fname;
            found=true;
            struct stat fsize;

            stat(fpath.c_str(), &fsize);

            ret=fsize.st_size/(float)1024;
        }

    }

    closedir(dpath);

    return ret;
}

int AS::NIXEngine::removeLock(){
    return unlink(commands["tool_lock_file"].c_str());
}

int AS::NIXEngine::cacheSize(){
    DIR *dpath = opendir(commands["download_path"].c_str());
    if(dpath==NULL) return 0;
    dirent *dir;
    std::string fname, fpath;
    struct stat fsize;
    float ret=0;

    while((dir=readdir(dpath))){
        fname=string(dir->d_name);
        fpath=commands["download_path"];
        fpath += fname;

        stat(fpath.c_str(), &fsize);

        ret += fsize.st_size;
    }

    closedir(dpath);

    return ret/(1024*1024);
}

int AS::NIXEngine::cleanCache(){
    return execCmd(commands["clean_cache"]);
}

std::string AS::NIXEngine::getNewsUrl(std::string lang){
    std::string base_key = "rss_main",key = "rss_";
    key+=lang;
    if(((std::string)sysinfo[key]).compare("")) return sysinfo[key];
    return sysinfo[base_key];
}

std::list<AS::Package*>* AS::NIXEngine::com_search(std::string pattern){
    std::list<AS::Package*>* ret = new std::list<AS::Package*>();

    std::string com_pattern="\"";
    com_pattern+=pattern;
    com_pattern+="\"";

    execComQuery(ret, com_pattern, false);

    return ret;
}

std::list<AS::Package*>* AS::NIXEngine::com_info(std::string pattern){
    std::list<AS::Package*>* ret = new std::list<AS::Package*>();

    execComQuery(ret, pattern, true);

    return ret;
}

int AS::NIXEngine::com_install(std::string pattern){
    string cmd(community["install"]), exec(community["cmd_exec"]);

    cmd += " ";
    cmd += pattern;

    return execCmd(cmd);
}

int AS::NIXEngine::com_remove(std::string pattern){
    string cmd(community["remove"]);

    cmd += " ";
    cmd += pattern;

    return execCmd(cmd);
}

int AS::NIXEngine::com_upgrade(std::string pattern){
    string cmd(community["upgrade"]);

    /*cmd += " ";
    cmd += pattern;*/

    return execCmd(cmd);
}

class ComQueryListener : public AS::EngineListener{
    regex_t filter;
    regex_t pkg_name;
    regex_t pkg_version;
    char sep;

    bool descr;

    std::list<AS::Package*>* pkgList;
public:
    ComQueryListener(std::list<AS::Package*>* pkgList, const char *filter, const char *pkgNameFilter, const char *pkgVersionFilter, char sep='/'){
        regcomp(&this->filter, filter, REG_EXTENDED);
        regcomp(&pkg_name, pkgNameFilter, REG_EXTENDED);
        regcomp(&pkg_version, pkgVersionFilter, REG_EXTENDED);

        this->pkgList = pkgList;

        this->sep=sep;

        descr=false;
    }

    ~ComQueryListener(){
        regfree(&filter);
        regfree(&pkg_name);
        regfree(&pkg_version);
    }

    void step(const char *content){
        if(descr){
            pkgList->back()->setDescription(content);
            descr=false;
            return;
        }

        regmatch_t match;
        string cstr(content), pname, version, repo;
        AS::Package *pkg = new AS::Package(false);

        if(!regexec(&filter, content, 1, &match, 0)){
            if(!regexec(&pkg_name, content, 1, &match, 0)){
                pname = cstr.substr(match.rm_so, match.rm_eo-match.rm_so);
                repo=pname.substr(0,pname.find(sep));
                pname=pname.substr(pname.find(sep)+1);

                if(!regexec(&pkg_version, content, 1, &match, 0)){
                    version = cstr.substr(match.rm_so, match.rm_eo-match.rm_so);

                    pkg->setName(pname);
                    pkg->setRepository(repo);
                    pkg->setRemoteVersion(version);

                    pkgList->insert(pkgList->end(), pkg);
                    descr=true;
                    return;
                }
            }
        }

        delete pkg;
    }
};

int AS::NIXEngine::execComQuery(std::list<AS::Package *> *pkgList, std::string pattern, bool info){    
    int status=0;

    if(!info){
        ComQueryListener ql(pkgList, regexps["query_filter_regexp"].c_str(), regexps["query_name_regexp"].c_str(), regexps["query_lversion_regexp"].c_str());
        addListener(&ql);
        string cmd = community["search"];

        cmd += " \"";
        cmd += pattern;
        cmd += "\"";

        status = execCmd(cmd);

        removeListener(&ql);
    }else{
        QueryInfoListener qil(pkgList, regexps["query_info_name_regexp"].c_str(), regexps["query_info_version_regexp"].c_str(), regexps["query_info_group_regexp"].c_str(),
                              regexps["query_info_license_regexp"].c_str(), regexps["query_info_description_regexp"].c_str(), regexps["query_info_url_regexp"].c_str(),
                              regexps["query_info_ksize_regexp"].c_str(),regexps["query_info_repo_regexp"].c_str(), true, as_EXPERT_QUERY);

        addListener(&qil);


        string cmd = community["query_info"];

        cmd += " ";
        cmd += pattern;

        status = execCmd(cmd);

        removeListener(&qil);
    }

    return status;
}
