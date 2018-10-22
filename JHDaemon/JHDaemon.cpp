// JHDaemon.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "common/Common.h"
#include "logfile/logfilewrapper.h"
#include "process/process.h"
#include "timer/TimerManager.h"
#include "xmlhelper/XmlHelper.h"

class AppInfo {
public:
    std::string id;             /* 标识 */
    std::string path;           /* 应用程序路径 */
    unsigned int rate;          /* 监听频率(秒) */
    bool alone;                 /* 是否运行在独立的控制台 */
    unsigned long pid;          /* 进程id */
};

static logfilewrapper_st* s_logWrapper = NULL;
static std::vector<AppInfo*> s_appInfoList;
static std::vector<Process> s_processList;

std::string nowdate(void) {
    struct tm t;
    time_t now;
    time(&now);
    localtime_s(&t, &now);
    char buf[32] = { 0 };
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return buf;
}

static bool isProcessExist(unsigned long pid) {
    for (size_t i = 0, len = s_processList.size(); i < len; ++i) {
        if (pid == s_processList[i].id) {
            return true;
        }
    }
    return false;
}

static unsigned long getAppProcessId(const std::string& appPath) {
    for (size_t i = 0, len = s_processList.size(); i < len; ++i) {
        if (appPath == s_processList[i].exePath() + s_processList[i].exeFile) {
            return s_processList[i].id;
        }
    }
    return 0;
}

static bool initLogFile(const std::string& logBasename, const std::string& logExtname) {
    if (s_logWrapper) {
        return true;
    }
    s_logWrapper = logfilewrapper_init(logBasename.c_str(), logExtname.c_str(), LOGFILE_DEFAULT_MAXSIZE, 1);
    if (s_logWrapper) {
        return true;
    }
    return false;
}

static void log(const std::string& str, bool withtime) {
    printf_s("%s", ((withtime ? "[" + nowdate() + "] " : "") + str).c_str());
    if (s_logWrapper) {
        logfilewrapper_record(s_logWrapper, NULL, 0, ((withtime ? "[" + nowdate() + "] " : "") + str).c_str());
    }
}

int main() {
    try {
        /* 初始日志文件 */
        const std::string logBasename = "JHDaemon";
        const std::string logExtname = ".log";
        if (!initLogFile(logBasename, logExtname)) {
            log("[ERROR] can not open JHDaemon.log\n", true);
            return 0;
        }
        /* 读取xml文件 */
        const std::string xmlFilename = "JHDaemon.xml";
        pugi::xml_document* doc = XmlHelper::loadFile(xmlFilename);
        if (!doc) {
            log("[ERROR] can not open JHDaemon.xml\n", true);
            return 0;
        }
        /* 解析xml配置文件 */
        pugi::xml_node root = XmlHelper::getNode(*doc, "root");
        if (root.empty()) {
            log("[ERROR] JHDaemon.xml not exist 'root' node\n", true);
            return 0;
        }
        std::vector<pugi::xml_node> children = XmlHelper::getChildren(root);
        if (children.empty()) {
            log("[ERROR] not exist application to listen\n", true);
            return 0;
        }
        log("==================== applications ====================\n", false);
        std::string currentDir = Common::replaceString(Common::getCurrentDir(), "\\", "/");
        for (size_t i = 0, len = children.size(); i < len; ++i) {
            char id[64] = { 0 };
            sprintf_s(id, "process_%03d", i + 1);
            std::string path = XmlHelper::getNodeText(children[i], "path").as_string();
            path = Common::replaceString(path, "\\", "/");
            if (!Common::isAbsolutePath(path.c_str())) {
                std::vector<std::string> currentDirVec = Common::splitString(currentDir, "/");
                if (!currentDirVec.empty()) {
                    currentDirVec.erase(currentDirVec.end() - 1);
                }
                std::vector<std::string> pathVec = Common::splitString(path, "/");
                std::vector<std::string>::iterator iter = pathVec.begin();
                while (pathVec.end() != iter) {
                    if (".." == *iter) {
                        pathVec.erase(iter);
                        if (!currentDirVec.empty()) {
                            currentDirVec.erase(currentDirVec.end() - 1);
                        }
                    } else {
                        ++iter;
                    }
                }
                path = "";
                for (size_t i1 = 0, len1 = currentDirVec.size(); i1 < len1; ++i1) {
                    path += currentDirVec[i1] + "/";
                }
                for (size_t i2 = 0, len2 = pathVec.size(); i2 < len2; ++i2) {
                    path += pathVec[i2] + (i2 < len2 - 1 ? "/" : "");
                }
            }
            unsigned int rate = XmlHelper::getNodeText(children[i], "rate").as_uint();
            if (0 == rate) {
                rate = 10;
            }
            bool alone = XmlHelper::getNodeText(children[i], "alone").as_bool(true);
            char rateBuf[16] = { 0 };
            sprintf_s(rateBuf, "%u", rate);
            std::string str = "---------- [" + std::string(id) + "]\n";
            str += "path: " + path + "\n";
            str += "rate: " + std::string(rateBuf) + "\n";
            str += "alone: " + std::string((alone ? "true" : "false")) + "\n";
            log(str, false);
            if (path.empty()) {
                continue;
            }
            AppInfo* ai = new AppInfo();
            ai->id = id;
            ai->path = path;
            ai->rate = rate;
            ai->alone = alone;
            ai->pid = 0;
            s_appInfoList.push_back(ai);
        }
        log("======================================================\n", false);
        /* 创建监听定时器 */
        if (s_appInfoList.empty()) {
            log("[ERROR] not exist valid application to listen\n", true);
            return 0;
        }
        s_processList = Process::getList();
        for (size_t j = 0, l = s_appInfoList.size(); j < l; ++j) {
            AppInfo* ai = s_appInfoList[j];
            if (0 == Process::isAppFileExist(ai->path.c_str())) {
                unsigned long pid = getAppProcessId(ai->path);
                if (0 == pid) {
                    int ret = Process::runApp(ai->path.c_str(), NULL, ai->alone, &pid);
                    if (0 == ret) {
                        log("Start application \"" + ai->path + "\", pid = [" + Common::toString((long)pid) + "]\n", true);
                    } else {
                        std::string str;
                        if (1 == ret) {
                            str = "path is NULL or empty";
                        } else if (2 == ret) {
                            str = "path is not absolute path";
                        } else if (3 == ret) {
                            str = "working directory is not absolute path";
                        } else if (4 == ret) {
                            str = "create process fail";
                        }
                        log("[ERROR] start application \"" + ai->path + "\" fail: " + str + " \n", true);
                    }
                } else {
                    log("Application \"" + ai->path + "\" has been started, pid = [" + Common::toString((long)pid) + "]\n", true);
                }
                ai->pid = pid;
            } else {
                log("[ERROR] not exist application file \"" + ai->path + "\"\n", true);
            }
            TimerManager::getInstance()->runLoop(ai->id.c_str(), ai->rate * 1000, [](timer_st* tm, unsigned long runCount, void* param)->void {
                AppInfo* ai = (AppInfo*)param;
                unsigned long pid = getAppProcessId(ai->path);
                if (pid > 0) {
                    if (ai->pid > 0 && pid != ai->pid) {
                        log("Application \"" + ai->path + "\" reassociate, old pid = [" + Common::toString((long)ai->pid) + "], new pid = [" + Common::toString((long)pid) + "]\n", true);
                    }
                    ai->pid = pid;
                    return;
                } else if (ai->pid > 0) {
                    log("[WARNING] application \"" + ai->path + "\", pid = [" + Common::toString((long)ai->pid) + "] has been ended\n", true);
                }
                ai->pid = 0;
                if (0 != Process::isAppFileExist(ai->path.c_str())) {
                    log("[ERROR] not exist application file \"" + ai->path + "\"\n", true);
                    return;
                }
                int ret = Process::runApp(ai->path.c_str(), NULL, ai->alone, &pid);
                if (0 == ret) {
                    log("Restart application \"" + ai->path + "\", pid = [" + Common::toString((long)pid) + "]\n", true);
                } else {
                    std::string str;
                    if (1 == ret) {
                        str = "path is NULL or empty";
                    } else if (2 == ret) {
                        str = "path is not absolute path";
                    } else if (3 == ret) {
                        str = "working directory is not absolute path";
                    } else if (4 == ret) {
                        str = "create process fail";
                    }
                    log("[ERROR] restart application \"" + ai->path + "\" fail: " + str + " \n", true);
                }
                ai->pid = pid;
            }, ai);
        }
        /* 主循环 */
        while (1) {
            Sleep(500);
            s_processList = Process::getList();
            TimerManager::getInstance()->update();
        }
    } catch (std::exception e) {
        log("[EXCEPTION] Application execption: " + std::string(e.what()) + "!!!\n", true);
    } catch (...) {
        log("[EXCEPTION] Application execption!!!\n", true);
    }
    return 0;
}
