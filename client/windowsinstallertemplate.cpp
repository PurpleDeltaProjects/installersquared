#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <thread>
#include <windows.h>
#include <json11.hpp>
#include <winhttp.h>
#include <webview/webview.h>


//declaring functions, definitions below main

bool check_admin();

std::vector<std::string> get_applist(const wchar_t* path);

std::string https_get(const wchar_t* domain, const wchar_t* path, const INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT);

json11::Json get_appinfo();

bool install_app(std::string name, std::string package_id, bool isadmin);

void webviewsetup(json11::Json& appinfo, std::string mode, webview::webview& w);



int wmain(int argc, wchar_t* argv[]) {

    //creates the webview object
    webview::webview w(true, nullptr);

    //if the app is not running with admin permissions:
    if(!check_admin()) {
        //open the app but with admin permissions. if it fails to open, print a message
        if((INT_PTR)ShellExecuteW(NULL, L"runas", argv[0], NULL, NULL, SW_SHOWNORMAL) < 33) {
            
            std::string mode = "Error: Administrator permissions required. Please reopen the application and press yes on the pop-up. Exiting...";
            auto placeholder = json11::Json();
            webviewsetup(placeholder, mode, w);
            w.run();
        }
        return 1;
    }

    //get the appinfo from the server
    auto appinfo = get_appinfo();

    if (appinfo.is_null()) {
        std::string mode = "Error: Data from the server was unable to be accessed. Please try again later. Exiting...";
        auto placeholder = json11::Json();
        webviewsetup(placeholder, mode, w);
        w.run();
        return 3;
    }

    //try to get the applist from the code of the app
    auto applist = get_applist(argv[0]);

    //removes all apps not in the appinfo from the applist
    applist.erase(std::remove_if(applist.begin(), applist.end(), [jsonmap = appinfo.object_items()](const std::string& app){
        return (jsonmap.find(app) == jsonmap.end());
    }), applist.end());

    //choose mode based on if the applist in app exists
    std::string mode = (applist.empty()) ? "dynamic" : "static";

    webviewsetup(appinfo, mode, w);

    w.run();

    return 0;
    
}


//this function checks if the current instance of the application is running as administrator
bool check_admin() {
    
    HANDLE token; //create a HANDLE variable for the function below to put the HANDLE of the process into (i think)
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false; //if the function fails, assume the process is not running as admin
    }

    TOKEN_ELEVATION elevation; //a variable that will hold whether or not the process is elevated
    DWORD requiredbytes; //this is pretty much useless for what im doing i think

    if (!GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &requiredbytes)) {
        CloseHandle(token);
        return false; //same as before, assume its not elevated
    }

    bool elevated = (elevation.TokenIsElevated != 0);

    CloseHandle(token);

    return elevated;

}


//this function finds the payload that contains the applist inside the executable, and returns it in vector form
std::vector<std::string> get_applist(const wchar_t* path) {

    //opens the file in read binary mode
    std::ifstream file(path, std::ios::binary);

    //if it didnt open, return nothing
    if (!file.is_open()) {
        return {};
    }

    //the marker and trailer of the payload
    std::string marker = "appliststart";
    std::string trailer = "applistend";

    //adds the dash outside of the string so that the code doesnt see the string literals above as the marker and trailer
    marker.replace(7,0,"---");
    trailer.replace(7,0,"---");

    //converts the executable into a string
    std::string filestring((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    //gets the start and end of the payload
    auto substart = filestring.rfind(marker);
    auto subend = filestring.rfind(trailer);

    //makes sure the marker and trailer positions make sense
    if (substart == std::string::npos || subend == std::string::npos || subend <= substart) {
        return {};
    }

    //adjust substart so that it starts after the marker
    substart += marker.length();

    //gets the applist in string and stream form
    std::string appstring = filestring.substr(substart, subend - substart);
    std::stringstream appstream(appstring);

    //create the needed app and applist vars
    std::string app;
    std::vector<std::string> applist;

    //loop over the applist in stream form, get a string for every entry, and add it to the applist vector
    while(std::getline(appstream, app, ',')) {
        applist.push_back(app);
    }

    return applist;

}


//this function returns the response from a server as a string, from its url
//only works for ascii data, if it fails it returns an empty string
std::string https_get(const wchar_t* domain, const wchar_t* path, const INTERNET_PORT port) {
    HINTERNET session = WinHttpOpen(
        L"InstallerSquaredClient/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session) {
        return "";
    }

    HINTERNET connect = WinHttpConnect(session, domain, port, 0);

    if (!connect) {
        WinHttpCloseHandle(session);
        return "";
    }

    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    BOOL sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (sent && WinHttpReceiveResponse(request, NULL)) {

        DWORD bytestoread;
        DWORD bytesread;
        std::string results;

        do {

            if(!WinHttpQueryDataAvailable(request, &bytestoread) || bytestoread == 0) {
                break;
            }

            //this is fine as a string as long as the json only contains ascii characters (it does for now)
            std::string buffer(bytestoread, '\0');

            if(!WinHttpReadData(request, &buffer[0], bytestoread, &bytesread)) {
                break;
            }

            results.append(buffer, 0, bytesread);
        
        } while (bytestoread > 0);

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        
        return results;

    } else {

        return "";

    }

}


//this function gets the app info from the server 
//and then returns a json object of the data in it, or null if there is an error
json11::Json get_appinfo() {

    const wchar_t* serverdomain = L"{{URL}}";

    const wchar_t* serverpath = L"/data/appinfo.json";

    //this returning an empty string wont make the rest of the function fail
    std::string jsondata = https_get(serverdomain, serverpath);

    std::string error;

    auto appinfo = json11::Json::parse(jsondata, error);

    return appinfo; //if json::parse fails, it will return null here, or the parsed json if it works

}


//this function installs the app with winget.
//needs the name of the app, its id, and whether it needs admin or not
bool install_app(std::string name, std::string package_id, bool admin) {

    auto arguments = std::wstring(L"install --id ") + std::wstring(package_id.begin(), package_id.end()) + L" --silent --accept-package-agreements --accept-source-agreements";

    //information that the shellexecuteex uses
    SHELLEXECUTEINFOW exinfo = {};

    //exinfo that is the same regardless of admin or not admin
    exinfo.cbSize = sizeof(exinfo);
    exinfo.hwnd = NULL;
    exinfo.lpVerb = L"open";
    exinfo.lpDirectory = NULL;
    exinfo.nShow = 0;
    exinfo.hInstApp = NULL;
    exinfo.lpFile = L"winget";
    exinfo.lpParameters = arguments.c_str();
    exinfo.fMask = SEE_MASK_NOCLOSEPROCESS;


    //define these strings for use in the no admin path functions
    std::wstring newtempfile;
    wchar_t check[MAX_PATH+1];

    //if admin, run winget directly
    if (admin) {

        if (!ShellExecuteExW(&exinfo)) {
            return false;
        }
        
    //otherwise, create a vbs file that runs the winget command, and run that through explorer.exe
    } else {
        
        //define strings for use in the below functions
        wchar_t tempfile[MAX_PATH+1];
        wchar_t temppath[MAX_PATH+1];

        //find the temp path
        if (GetTempPathW(MAX_PATH+1, temppath) == 0) {
            return false;
        }

        //create the check file
        if (GetTempFileNameW(temppath, L"IS", 0, check) == 0) {
            return false;
        }

        //create the temp vbs
        if (GetTempFileNameW(temppath, L"IS", 0, tempfile) == 0) {
            return false;
        }

        //rename the vbs to the correct extension
        newtempfile = (std::wstring(tempfile) + L".vbs");

        if(MoveFileW(tempfile, newtempfile.c_str()) == 0) {
            return false;
        }

        //make the vbs file have the correct code
        std::wofstream vbsfile(newtempfile.c_str());

        if (vbsfile.is_open()) {

            std::wstring filestring = 
            L"Set Shell = CreateObject(\"WScript.Shell\")\n"
            L"Shell.Run \"winget " + arguments + "\", 0, True\n"
            L"Set Shell = Nothing\n"
            L"Set FSO = CreateObject(\"Scripting.FileSystemObject\")\n"
            L"FSO.DeleteFile \"" + std::wstring(check) + L"\"\n"
            L"Set FSO = Nothing\n";

            vbsfile << filestring;

            vbsfile.close();

        } else {
            return false;
        }

        std::wstring tempparameter = L"\"" + newtempfile + L"\"";

        //run it with this function, rather than ex 
        if((INT_PTR)ShellExecuteW(NULL, L"open", L"explorer", tempparameter.c_str(), NULL, 0) < 33) {
            return false;
        }


    }

    std::cout << "Installing: " << name << std::endl;

    if(!admin) {

        //wait until the vbs script is done running (check file gets deleted at end)
        while (std::filesystem::exists(check)) {
            Sleep(10);
        }

        //delete the other temp file
        DeleteFileW(newtempfile.c_str());

    

        //set arguments for winget app check
        auto check_arguments = std::wstring(L"list --id ") + std::wstring(package_id.begin(), package_id.end());

        //change this for the new arguments
        exinfo.lpParameters = check_arguments.c_str();

        //run the winget command again, to check success
        if (!ShellExecuteExW(&exinfo)) {
            //if it made it up to this point without failing, assume success until proven otherwise
            return true;
    }

    }

    //check and cleanup of the process (both paths)
    WaitForSingleObject(exinfo.hProcess, INFINITE);

    DWORD exitcode;

    GetExitCodeProcess(exinfo.hProcess, &exitcode);

    CloseHandle(exinfo.hProcess);

    if (exitcode == 0 || exitcode == -1978335189) { //exit code 0 is success and the other is app already installed
        return true;
    }

    return false;

}


//this function sets up the webview gui
void webviewsetup(json11::Json& appinfo, std::string mode, webview::webview& w) {

    //gui mode string
    const std::string html = std::string("data:text/html,") + R"({{HTML}})";

    //create the gui
    w.set_title("InstallerSquared");
    w.set_size(1280, 720, WEBVIEW_HINT_NONE);

    //allow the javascript to know which mode to run
    w.bind("getMode", [mode](std::string a=""){return "\""+ mode + "\"";});

    //allow the javascript to run apps
    w.bind("downloadApp", 
        [&appinfo, &w](std::string appname){

            //remove the [" and "] from the returned appname
            std::string app = appname.substr(2, appname.size() - 4);

            std::string name = appinfo[app]["name"].string_value();

            std::string package_id = appinfo[app]["package-id"].string_value();

            bool admin = !appinfo[app]["no-admin"].bool_value();

            std::thread([&w, admin, name, package_id]() {
                bool success = install_app(name, package_id, admin); 
                w.dispatch([&w, success]() {
                    w.eval("complete = true");
                    w.eval(std::string("success = ") + std::string((success) ? "true" : "false"));
                });
                
            }).detach();

            return "";
            
        });

    //create a function allowing the javascript to get the appinfo
    w.bind("getAppInfo", 
        [appinfo = appinfo.dump()](std::string a=""){return appinfo;}
    );

    w.bind("closeApp", 
        [&w](std::string a=""){w.terminate();return "";}
    );

    //open the html file python puts in the string
    w.navigate(html);

}