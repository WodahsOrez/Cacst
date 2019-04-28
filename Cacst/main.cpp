#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curl.h"
#include <windows.h>
#include <string>
#include <direct.h>
#include <cstdio>
#include <io.h>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;

std::string GetCmdInfo(const char * pszCmd)
{
    //创建匿名管道
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        return "";
    }

    //设置命令行进程启动信息(以隐藏方式启动命令并定位其输出到hWrite)
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;

    //启动命令行
    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, (char *)pszCmd, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        return "";
    }

    //立即关闭hWrite
    CloseHandle(hWrite);

    //读取命令行返回值
    std::string strRet;
    char buff[1024] = {0};
    DWORD dwRead = 0;
    while (ReadFile(hRead, buff, 1024, &dwRead, NULL))
    {
        strRet.append(buff, dwRead);
    }
    CloseHandle(hRead);

    return strRet;
}

//字符转整型
char Char2Int(char ch)
{
    if(ch>='0' && ch <='9')return (char)(ch-'0');
    if(ch>='a' && ch <='f')return (char)(ch-'a'+10);
    if(ch>='A' && ch <='F')return (char)(ch-'A'+10);
    return -1;
}

//URL解码
string urlDecode(string str)
{
    string output = "";
    char tmp[2];
    int i=0,len=str.length();
    while(i<len){
        if(str[i]=='%'){
            tmp[0]=Char2Int(str[i+1]);
            tmp[1]=Char2Int(str[i+2]);
            output += (tmp[0] << 4)| tmp[1];
            i=i+3;
        }
        else if (str[i]=='+'){
            output+=' ';
            i++;
        }
        else{
            output+=str[i];
            i++;
        }
    }
    return output;
}

//字符串分割到数组
void Split(const string& src, const string& separator, vector<string>& dest)
{
    string str = src;
    string substring;
    string::size_type start = 0, index;
    dest.clear();
    index = str.find(separator,start);
    do
    {
        if (index != string::npos)
        {
            substring = str.substr(start,index-start);
            dest.push_back(substring);
            start = index + separator.size();
            index = str.find(separator,start);
            if(start == string::npos)break;
        }
    }while(index != string::npos);

    //最后一部分
    substring = str.substr(start);
    dest.push_back(substring);
}

// 执行cmd命令,并将结果存储到result字符串数组中
int execmd(const char* cmd, char* result)
{
    cout << cmd << endl;                //输出命令
    memset(result,0,strlen(result));
    char buffer[1024];                  //定义缓存区
    FILE* pipe = _popen(cmd, "r");      //打开管道执行命令
    if (!pipe)
        return 0;                       //返回0表示运行失败

    while (fgets(buffer, 1024, pipe) != NULL)
    {
        strcat(result,buffer);          //将管道输出到result中
    }
    cout << "result: " << result << endl;             //输出返回值
    _pclose(pipe);                      //关闭管道
    return 1;                           //返回1表示运行成功
}

//UTF-8转GB2312
char* U2G(const char* utf8)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len+1];
    memset(wstr, 0, len+1);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len+1];
    memset(str, 0, len+1);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
    if(wstr) delete[] wstr;
    return str;
}

//GB2312转UTF-8
char* G2U(const char* gb2312)
{
    int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len+1];
    memset(wstr, 0, len+1);
    MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len+1];
    memset(str, 0, len+1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    if(wstr) delete[] wstr;
    return str;
}

//获取exe所在的路径
string getExePath()
{
    char fullPath[MAX_PATH];
    GetModuleFileNameA(NULL, fullPath, MAX_PATH);
    string strPath = (string)fullPath;
    return strPath.substr(0,strPath.find_last_of('\\'));
}


//解析url获取相应参数值
string getParam(const string path, const string param)
{
    size_t first = path.find(param);
    if (first != path.npos)
    {
        size_t end = path.find("&",first);
        size_t pos = first+param.length()+1;
        return path.substr(pos,end-pos);
    }
    return "";
}

// 获取文件夹下的所有文件名,format可以定义后缀
void getAllFiles(const string path,vector<string>& files,const string format)
{
    long hFile = 0;//文件句柄
    struct _finddata_t fileinfo; //文件信息
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\" + format).c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) // 文件夹名中不包含"."和".."
                {
                    //files.push_back(p.assign(path).append("\\").append(fileinfo.name)); //保存文件夹名
                    getAllFiles(p.assign(path).append("\\").append(fileinfo.name),files,format); // 递归遍历子文件夹
                }
            }
            else
            {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name)); //存储非文件夹的文件名
            }
        }while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}


//打开文件(若路径不存在则创建,可创建多级目录)
FILE* file_open(const char* path,const char* mode)
{
    FILE *fp = fopen(path,mode);
    cout << "是否需要创建文件夹:" << !fp <<endl;
    if(!fp)
    {
        string s(path);
        string cmdstr = "md ";
        cmdstr += s.substr(0,s.rfind('\\',s.length()));
        fprintf(stdout,cmdstr.c_str());
        system(cmdstr.c_str());
        cout << "文件夹创建" << endl;
        fp = fopen(path,mode);
    }
    return fp;
}

//下载数据回调
size_t callback_download_file(void *ptr, size_t size, size_t nmemb, void *userp)
 {
    return fwrite(ptr,size,nmemb,(FILE *)userp);      //必须返回这个大小, 否则只回调一次, 不清楚为何.
 }

 //下载服务器文件
 void *func_download_file(const char* url,const char* localpath,const char* remotepath)
 {
    cout << "下载开始" << endl;
    FILE *fp = file_open(localpath,"wb");     //检查路径并创建文件输出流
    CURL *curl = NULL;
    CURLcode res;


    curl = curl_easy_init();    //获得curl指针
    curl_easy_setopt(curl, CURLOPT_URL, url); //设置下载的URI
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);        //设置超时
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //屏蔽其它信号
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);        //启用内部debug日志
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);//设定为不验证证书和HOST
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback_download_file); //设置下载数据的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);      //设置回调函数最后一个参数
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stdout,curl_easy_strerror(res));
        fprintf(stdout,"\ncurl download fail\n");
    }
    curl_easy_cleanup(curl);
    fclose(fp);

    // 移动端传输
    char result[1024*3] = "";           //创建接收返回信息的字符数组
    string cmd = getExePath()+"\\adb get-state";
    //execmd(cmd.c_str(),result);
    if ((GetCmdInfo(cmd.c_str())).find("error:",0) != string::npos)
    {
        cout << "未检测到移动设备" << endl;
        MessageBox(GetForegroundWindow(),"未检测到移动设备","Cacst",MB_OK);
    }
    else
    {
        cout << "开始传输文件至移动设备" << endl;
        cmd = getExePath()+"\\adb push "+localpath+" "+remotepath;
        cout << cmd << endl;
        system(cmd.c_str());
    }

    cout << "开始删除本机文件" << endl;
    cout << localpath << endl;
    DeleteFile(localpath);

    cout << "下载结束" << endl;

    return 0;
 }

 //上传数据回调
size_t callback_upload_file(void *ptr, size_t size, size_t nmemb, void *userp)
 {
    string* info = (string*)userp;
    info->append((char*)ptr,size * nmemb);
    return (size * nmemb);     //必须返回这个大小, 否则只回调一次, 不清楚为何.
 }

//用post/form提交文件数据到服务器
void *func_upload_file(const char* url,const char *filepath,const char* dir,const char* username){
    cout << "上传开始" << endl;
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost = 0;
    struct curl_httppost *lastptr  = 0;
    struct curl_slist *headers = NULL;
    string info = "";
    //headers = curl_slist_append(headers,"content-type:multipart/form-data;charset=ISO8859-1");



    //post form的设置
    //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME,
    //        "username", CURLFORM_PTRCONTENTS, "admin", CURLFORM_END);    //普通input字段,暂时用不到

    // 移动端传输
    char result[1024*3] = "";           //创建接收返回信息的字符数组
    string cmd = getExePath()+"\\adb get-state";
    //execmd(cmd.c_str(),result);
    vector<string> files;
    bool flag = true;
    if (GetCmdInfo(cmd.c_str()).find("no devices",0) != string::npos)
    {
        cout << "未检测到移动设备" << endl;
        MessageBox(GetForegroundWindow(),"未检测到移动设备","Cacst",MB_OK);
    }
    else
    {
        cmd = getExePath()+"\\adb ls /sdcard/jjxt/upload/"+dir+"/"+username;
        execmd(cmd.c_str(),result);
        Split(result,"\n",files);
        for (int i = 0;i < (int)files.size(); i++)
        {
            string filename = files[i].substr(files[i].find_last_of(' ')+1);
            if (!(filename == "." || filename == ".." || filename == "\0"))
            {
                if (filename.find("success.json",0) == string::npos)
                {
                    flag = false;
                    cmd = getExePath()+"\\adb pull /sdcard/jjxt/upload/"+dir+"/"+username+"/"+filename+" "+getExePath()+"\\upload\\"+dir+"\\"+username+"\\"+filename;
                    cout << cmd << endl;
                    string sss = "md "+getExePath()+"\\upload\\"+dir+"\\"+username;
                    system(sss.c_str());
                    system(cmd.c_str());
                    //cout << GetCmdInfo(cmd.c_str()) << endl;
                }
            }
        }
    }
    if(flag)
    {
        cout << "未检测到需要上传的文件" << endl;
        MessageBox(GetForegroundWindow(),"未检测到需要上传的文件","Cacst",MB_OK);
        return 0;
    }


    //遍历文件夹下的文件并上传
    vector<string> fileNames;
    getAllFiles(filepath,fileNames,"*");  //最后一个参数是匹配规则,支持通配符*和?
    for(int i = 0; i < (int)fileNames.size(); i++)
    {
        cout << "file:"<< fileNames[i].c_str() <<"."<< endl;
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME,
           G2U(fileNames[i].c_str()) , CURLFORM_FILE, fileNames[i].c_str() , CURLFORM_END);   //传入的文件
    }

    //curl的设置
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);        //内部Debug日志
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback_upload_file); //设置下载数据的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &info);      //设置回调函数最后一个参数

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stdout,curl_easy_strerror(res));
        fprintf(stdout,"\ncurl upload fail\n");
    }

    // 删除操作
    if (info.find("success",0) != string::npos)
    {
        cout << info << endl;
        cout << info.find("success",0) << endl;
        cout << "开始删除移动端数据" << endl;
        for (int i = 0;i < (int)files.size(); i++)
        {
            string filename = files[i].substr(files[i].find_last_of(' ')+1);
            if (!(filename == "." || filename == ".." || filename == "\0"))
            {
                cmd = getExePath()+"\\adb shell rm /sdcard/jjxt/upload/"+dir+"/"+username+"/"+filename;
                execmd(cmd.c_str(),result);
            }
        }
        // 下载上传成功数据
        string url2 = string(url);
        url2 = url2.substr(0,url2.find("upload.do",0))+"downMessage.do?dir="+dir;
        string returnfile;
        if (strcmp(dir,"military") == 0)
        {
            returnfile ="militaryInspection_success.json";
        }
        else if  (strcmp(dir,"patrol") == 0)
        {
            returnfile ="patrolMsg_success.json";
        }
        else if  (strcmp(dir,"qualityproblem") == 0)
        {
            returnfile ="qualityProblem_success.json";
        }
        url2 = url2 +"&filename="+ returnfile;
        string localpath = getExePath()+"\\upload\\"+dir+"\\"+username+"\\"+returnfile;
        string remotepath = string("/sdcard/jjxt/upload/") + dir + "/"+username+"/"+returnfile;
        func_download_file(url2.c_str(),localpath.c_str(),remotepath.c_str());
    }
    else if(!info.empty())
    {
        cout << info << endl;
        MessageBox(GetForegroundWindow(),info.c_str(),"Cacst",MB_OK);
    }

    // 上传结束后删除本机文件
    for(int i = 0; i < (int)fileNames.size(); i++)
    {
        cout << "开始删除本机文件" << endl;
        cout << fileNames[i].c_str() << endl;
        DeleteFile(fileNames[i].c_str());
    }
    MessageBox(GetForegroundWindow(),"上传结束，请刷新页面","Cacst",MB_OK);
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    return 0;
}

int main(int argc,char *argv[])
{
     // 移动端验证
    string cmd = getExePath()+"\\adb get-state";
    if ((GetCmdInfo(cmd.c_str())).find("error:",0) != string::npos)
    {
        cout << "未检测到移动设备" << endl;
        MessageBox(GetForegroundWindow(),"未检测到移动设备","Cacst",MB_OK);
        return 0;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);   //初始化curl,只执行一次
    string url(argv[argc-1]);
    url = "http" + url.substr(5);            //读取url并替换开头协议为https
    cout << "url:" << url << endl;
    if (url.find("/transfer/download.do") != url.npos)      //下载功能操作
    {
        string dir = getParam(url,"dir");
        cout << "dir:" << dir << endl;
        string username = getParam(url,"username");
        cout << "username:" << username << endl;
        string jsonname = getParam(url,"jsonname");
        cout << "jsonname:" << jsonname << endl;
        string localpath = getExePath()+"\\download\\"+dir+"\\"+username+"\\"+jsonname;
        cout << "localpath:" << localpath << endl;
        string remotepath = "/sdcard/jjxt/download/"+dir+"/"+username+"/"+jsonname;
        cout << "remotepath:" << remotepath << endl;
        func_download_file(url.c_str(),localpath.c_str(),remotepath.c_str());

        // 下载文件处理
        string url2 = url.substr(0,url.find("download.do",0))+"downfile.do";
        cout << "url2:" << url2 << endl;
        string filename = getParam(url,"filename");
        cout << "filename:" << filename << endl;
        cout << "filename empty:" << (filename == "") << endl;
        if (filename != "")
        {
            vector<string> filenames;
            Split(filename,",",filenames);
            for (int i = 0;i < (int)filenames.size(); i++)
            {
                string url3 = url2 +"?dir="+dir+"&filename="+filenames[i];
                cout << "url3:" << url3 << endl;
                char * fileGBK = U2G(urlDecode(filenames[i]).c_str());
                string localpath = getExePath()+"\\download\\"+dir+"\\files\\"+fileGBK;
                cout << "localpath:" << localpath << endl;
                string remotepath = "";
                if (strcmp(dir.c_str(),"military") == 0)
                {
                    remotepath = "/sdcard/jjxt/upload/"+dir+"/"+username+"/"+fileGBK;
                }
                else
                {
                    remotepath = "/sdcard/jjxt/download/"+dir+"/files/"+fileGBK;
                }
                cout << "remotepath:" << remotepath << endl;
                func_download_file(url3.c_str(),localpath.c_str(),remotepath.c_str());
            }
        }
        MessageBox(GetForegroundWindow(),"下载结束","Cacst",MB_OK);

    }
    else if (url.find("/transfer/upload.do") != url.npos)       //上传功能
    {
        string dir = getParam(url,"dir");
        cout << "dir:" << dir << endl;
        string username = getParam(url,"username");
        cout << "username:" << username << endl;
        string filepath = getExePath() + "\\upload\\" + dir;
        cout << "filepath:" << filepath << endl;
        func_upload_file(url.c_str(),filepath.c_str(),dir.c_str(),username.c_str());
    }
    else if (url.find("/transfer/userinfo.do") != url.npos)       //下载用户信息
    {
        // 下载用户信息
        string localpath = getExePath()+"\\download\\userinfo\\userinfo.json";
        cout << "localpath:" << localpath << endl;
        string remotepath = "/sdcard/jjxt/download/userinfo/userinfo.json";
        cout << "remotepath:" << remotepath << endl;
        func_download_file(url.c_str(),localpath.c_str(),remotepath.c_str());
        // 下载基础信息
        string url2 = url.substr(0,url.find("userinfo.do",0))+"basicmsg.do";
        string localpath2 = getExePath()+"\\download\\basicmsg\\basicmsg.json";
        cout << "localpath:" << localpath << endl;
        string remotepath2 = "/sdcard/jjxt/download/basicmsg/basicmsg.json";
        cout << "remotepath:" << remotepath << endl;
        func_download_file(url2.c_str(),localpath2.c_str(),remotepath2.c_str());
        MessageBox(GetForegroundWindow(),"下载结束","Cacst",MB_OK);
    }


    curl_global_cleanup();      //对curl_global_init做的工作清理。类似于close的函数。
    system("pause");
    return 0;
}
