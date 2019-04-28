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
    //���������ܵ�
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        return "";
    }

    //���������н���������Ϣ(�����ط�ʽ���������λ�������hWrite)
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;

    //����������
    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, (char *)pszCmd, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        return "";
    }

    //�����ر�hWrite
    CloseHandle(hWrite);

    //��ȡ�����з���ֵ
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

//�ַ�ת����
char Char2Int(char ch)
{
    if(ch>='0' && ch <='9')return (char)(ch-'0');
    if(ch>='a' && ch <='f')return (char)(ch-'a'+10);
    if(ch>='A' && ch <='F')return (char)(ch-'A'+10);
    return -1;
}

//URL����
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

//�ַ����ָ����
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

    //���һ����
    substring = str.substr(start);
    dest.push_back(substring);
}

// ִ��cmd����,��������洢��result�ַ���������
int execmd(const char* cmd, char* result)
{
    cout << cmd << endl;                //�������
    memset(result,0,strlen(result));
    char buffer[1024];                  //���建����
    FILE* pipe = _popen(cmd, "r");      //�򿪹ܵ�ִ������
    if (!pipe)
        return 0;                       //����0��ʾ����ʧ��

    while (fgets(buffer, 1024, pipe) != NULL)
    {
        strcat(result,buffer);          //���ܵ������result��
    }
    cout << "result: " << result << endl;             //�������ֵ
    _pclose(pipe);                      //�رչܵ�
    return 1;                           //����1��ʾ���гɹ�
}

//UTF-8תGB2312
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

//GB2312תUTF-8
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

//��ȡexe���ڵ�·��
string getExePath()
{
    char fullPath[MAX_PATH];
    GetModuleFileNameA(NULL, fullPath, MAX_PATH);
    string strPath = (string)fullPath;
    return strPath.substr(0,strPath.find_last_of('\\'));
}


//����url��ȡ��Ӧ����ֵ
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

// ��ȡ�ļ����µ������ļ���,format���Զ����׺
void getAllFiles(const string path,vector<string>& files,const string format)
{
    long hFile = 0;//�ļ����
    struct _finddata_t fileinfo; //�ļ���Ϣ
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\" + format).c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) // �ļ������в�����"."��".."
                {
                    //files.push_back(p.assign(path).append("\\").append(fileinfo.name)); //�����ļ�����
                    getAllFiles(p.assign(path).append("\\").append(fileinfo.name),files,format); // �ݹ�������ļ���
                }
            }
            else
            {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name)); //�洢���ļ��е��ļ���
            }
        }while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}


//���ļ�(��·���������򴴽�,�ɴ����༶Ŀ¼)
FILE* file_open(const char* path,const char* mode)
{
    FILE *fp = fopen(path,mode);
    cout << "�Ƿ���Ҫ�����ļ���:" << !fp <<endl;
    if(!fp)
    {
        string s(path);
        string cmdstr = "md ";
        cmdstr += s.substr(0,s.rfind('\\',s.length()));
        fprintf(stdout,cmdstr.c_str());
        system(cmdstr.c_str());
        cout << "�ļ��д���" << endl;
        fp = fopen(path,mode);
    }
    return fp;
}

//�������ݻص�
size_t callback_download_file(void *ptr, size_t size, size_t nmemb, void *userp)
 {
    return fwrite(ptr,size,nmemb,(FILE *)userp);      //���뷵�������С, ����ֻ�ص�һ��, �����Ϊ��.
 }

 //���ط������ļ�
 void *func_download_file(const char* url,const char* localpath,const char* remotepath)
 {
    cout << "���ؿ�ʼ" << endl;
    FILE *fp = file_open(localpath,"wb");     //���·���������ļ������
    CURL *curl = NULL;
    CURLcode res;


    curl = curl_easy_init();    //���curlָ��
    curl_easy_setopt(curl, CURLOPT_URL, url); //�������ص�URI
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);        //���ó�ʱ
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //���������ź�
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);        //�����ڲ�debug��־
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);//�趨Ϊ����֤֤���HOST
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback_download_file); //�����������ݵĻص�����
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);      //���ûص��������һ������
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stdout,curl_easy_strerror(res));
        fprintf(stdout,"\ncurl download fail\n");
    }
    curl_easy_cleanup(curl);
    fclose(fp);

    // �ƶ��˴���
    char result[1024*3] = "";           //�������շ�����Ϣ���ַ�����
    string cmd = getExePath()+"\\adb get-state";
    //execmd(cmd.c_str(),result);
    if ((GetCmdInfo(cmd.c_str())).find("error:",0) != string::npos)
    {
        cout << "δ��⵽�ƶ��豸" << endl;
        MessageBox(GetForegroundWindow(),"δ��⵽�ƶ��豸","Cacst",MB_OK);
    }
    else
    {
        cout << "��ʼ�����ļ����ƶ��豸" << endl;
        cmd = getExePath()+"\\adb push "+localpath+" "+remotepath;
        cout << cmd << endl;
        system(cmd.c_str());
    }

    cout << "��ʼɾ�������ļ�" << endl;
    cout << localpath << endl;
    DeleteFile(localpath);

    cout << "���ؽ���" << endl;

    return 0;
 }

 //�ϴ����ݻص�
size_t callback_upload_file(void *ptr, size_t size, size_t nmemb, void *userp)
 {
    string* info = (string*)userp;
    info->append((char*)ptr,size * nmemb);
    return (size * nmemb);     //���뷵�������С, ����ֻ�ص�һ��, �����Ϊ��.
 }

//��post/form�ύ�ļ����ݵ�������
void *func_upload_file(const char* url,const char *filepath,const char* dir,const char* username){
    cout << "�ϴ���ʼ" << endl;
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost = 0;
    struct curl_httppost *lastptr  = 0;
    struct curl_slist *headers = NULL;
    string info = "";
    //headers = curl_slist_append(headers,"content-type:multipart/form-data;charset=ISO8859-1");



    //post form������
    //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME,
    //        "username", CURLFORM_PTRCONTENTS, "admin", CURLFORM_END);    //��ͨinput�ֶ�,��ʱ�ò���

    // �ƶ��˴���
    char result[1024*3] = "";           //�������շ�����Ϣ���ַ�����
    string cmd = getExePath()+"\\adb get-state";
    //execmd(cmd.c_str(),result);
    vector<string> files;
    bool flag = true;
    if (GetCmdInfo(cmd.c_str()).find("no devices",0) != string::npos)
    {
        cout << "δ��⵽�ƶ��豸" << endl;
        MessageBox(GetForegroundWindow(),"δ��⵽�ƶ��豸","Cacst",MB_OK);
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
        cout << "δ��⵽��Ҫ�ϴ����ļ�" << endl;
        MessageBox(GetForegroundWindow(),"δ��⵽��Ҫ�ϴ����ļ�","Cacst",MB_OK);
        return 0;
    }


    //�����ļ����µ��ļ����ϴ�
    vector<string> fileNames;
    getAllFiles(filepath,fileNames,"*");  //���һ��������ƥ�����,֧��ͨ���*��?
    for(int i = 0; i < (int)fileNames.size(); i++)
    {
        cout << "file:"<< fileNames[i].c_str() <<"."<< endl;
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME,
           G2U(fileNames[i].c_str()) , CURLFORM_FILE, fileNames[i].c_str() , CURLFORM_END);   //������ļ�
    }

    //curl������
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);        //�ڲ�Debug��־
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback_upload_file); //�����������ݵĻص�����
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &info);      //���ûص��������һ������

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stdout,curl_easy_strerror(res));
        fprintf(stdout,"\ncurl upload fail\n");
    }

    // ɾ������
    if (info.find("success",0) != string::npos)
    {
        cout << info << endl;
        cout << info.find("success",0) << endl;
        cout << "��ʼɾ���ƶ�������" << endl;
        for (int i = 0;i < (int)files.size(); i++)
        {
            string filename = files[i].substr(files[i].find_last_of(' ')+1);
            if (!(filename == "." || filename == ".." || filename == "\0"))
            {
                cmd = getExePath()+"\\adb shell rm /sdcard/jjxt/upload/"+dir+"/"+username+"/"+filename;
                execmd(cmd.c_str(),result);
            }
        }
        // �����ϴ��ɹ�����
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

    // �ϴ�������ɾ�������ļ�
    for(int i = 0; i < (int)fileNames.size(); i++)
    {
        cout << "��ʼɾ�������ļ�" << endl;
        cout << fileNames[i].c_str() << endl;
        DeleteFile(fileNames[i].c_str());
    }
    MessageBox(GetForegroundWindow(),"�ϴ���������ˢ��ҳ��","Cacst",MB_OK);
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    return 0;
}

int main(int argc,char *argv[])
{
     // �ƶ�����֤
    string cmd = getExePath()+"\\adb get-state";
    if ((GetCmdInfo(cmd.c_str())).find("error:",0) != string::npos)
    {
        cout << "δ��⵽�ƶ��豸" << endl;
        MessageBox(GetForegroundWindow(),"δ��⵽�ƶ��豸","Cacst",MB_OK);
        return 0;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);   //��ʼ��curl,ִֻ��һ��
    string url(argv[argc-1]);
    url = "http" + url.substr(5);            //��ȡurl���滻��ͷЭ��Ϊhttps
    cout << "url:" << url << endl;
    if (url.find("/transfer/download.do") != url.npos)      //���ع��ܲ���
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

        // �����ļ�����
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
        MessageBox(GetForegroundWindow(),"���ؽ���","Cacst",MB_OK);

    }
    else if (url.find("/transfer/upload.do") != url.npos)       //�ϴ�����
    {
        string dir = getParam(url,"dir");
        cout << "dir:" << dir << endl;
        string username = getParam(url,"username");
        cout << "username:" << username << endl;
        string filepath = getExePath() + "\\upload\\" + dir;
        cout << "filepath:" << filepath << endl;
        func_upload_file(url.c_str(),filepath.c_str(),dir.c_str(),username.c_str());
    }
    else if (url.find("/transfer/userinfo.do") != url.npos)       //�����û���Ϣ
    {
        // �����û���Ϣ
        string localpath = getExePath()+"\\download\\userinfo\\userinfo.json";
        cout << "localpath:" << localpath << endl;
        string remotepath = "/sdcard/jjxt/download/userinfo/userinfo.json";
        cout << "remotepath:" << remotepath << endl;
        func_download_file(url.c_str(),localpath.c_str(),remotepath.c_str());
        // ���ػ�����Ϣ
        string url2 = url.substr(0,url.find("userinfo.do",0))+"basicmsg.do";
        string localpath2 = getExePath()+"\\download\\basicmsg\\basicmsg.json";
        cout << "localpath:" << localpath << endl;
        string remotepath2 = "/sdcard/jjxt/download/basicmsg/basicmsg.json";
        cout << "remotepath:" << remotepath << endl;
        func_download_file(url2.c_str(),localpath2.c_str(),remotepath2.c_str());
        MessageBox(GetForegroundWindow(),"���ؽ���","Cacst",MB_OK);
    }


    curl_global_cleanup();      //��curl_global_init���Ĺ�������������close�ĺ�����
    system("pause");
    return 0;
}
