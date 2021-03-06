#include "common.h"
#include "procCommServ.h"
#include "procMgr.h"
#include "inject.h"
#include "proxyConfig.h"
#include "clientMain.h"
#include "netpool.h"

#include "Mswsock.h"
#include "XPProgDlg.h"

MUTEX_TYPE g_prox_proc_lock;
PROC_LIST g_prox_proc_objs;

const char* g_proxy_proc_status_desc[] =
{
    "init",
    "loaded",
    "proxing",
    "unloaded"
};

static bool proxy_proc_is_exist(uint64_t proc_id)
{
    PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    MUTEX_LOCK(g_prox_proc_lock);
    for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
    {
        proxy_proc = *itr;

        if (proxy_proc->proc_id == proc_id)
        {
        	MUTEX_UNLOCK(g_prox_proc_lock);
        	return true;
        }
    }
    MUTEX_UNLOCK(g_prox_proc_lock);
    return false;
}

static void proxy_proc_update(uint64_t proc_id)
{
	PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    uint64_t cur_time = util_get_cur_time();

    MUTEX_LOCK(g_prox_proc_lock);
    for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
    {
        proxy_proc = *itr;

        if (proxy_proc->proc_id == proc_id)
        {
            if (proxy_proc->is_injected == false)
            {
                if (cur_time - proxy_proc->update_time >= 2)
                {                    
                    bool isInjected = false;
                    if (0 == is_dll_injected(proxy_proc->proc_name, proxy_proc->proc_id, &isInjected)
                        && (isInjected == true))
                    {
                        unInject_dll(proxy_proc->proc_name, proxy_proc->proc_id);
                    }

                    if (0 == inject_dll(proxy_proc->proc_name, proxy_proc->proc_id))
                    {
                        proxy_proc->status = PROC_ST_INJECTED;
                        proxy_proc->is_injected = true;

                        /*通知主窗口更新*/
                        ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
                    }
                }
            }

        	proxy_proc->update_time = cur_time;

        	//_LOG_DEBUG(_T("update proxy process: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
        	MUTEX_UNLOCK(g_prox_proc_lock);
        	return;
        }
    }
    MUTEX_UNLOCK(g_prox_proc_lock);
}


void proxy_proc_set_registered(uint64_t proc_id)
{
    PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    MUTEX_LOCK(g_prox_proc_lock);
    for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
    {
        proxy_proc = *itr;

        if (proxy_proc->proc_id == proc_id)
        {
            proxy_proc->status = PROC_ST_REGISTED;
            _LOG_INFO(_T("set proxy process to registered: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
            MUTEX_UNLOCK(g_prox_proc_lock);

            /*通知主窗口更新*/
            ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
            return;
        }
    }
    MUTEX_UNLOCK(g_prox_proc_lock);
    return;
}

void proxy_proc_set_uninjected(uint64_t proc_id)
{
    PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    MUTEX_LOCK(g_prox_proc_lock);
    for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
    {
        proxy_proc = *itr;

        if (proxy_proc->proc_id == proc_id)
        {
            proxy_proc->status = PROC_ST_UNINJECTED;
            proxy_proc->is_injected = false;
            _LOG_INFO(_T("set proxy process to uninjected: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
            MUTEX_UNLOCK(g_prox_proc_lock);

            /*通知主窗口更新*/
            ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
            return;
        }
    }
    MUTEX_UNLOCK(g_prox_proc_lock);
    return;
}

int proxy_proc_get_injected_cnt()
{
	int cnt = 0;

	PROC_LIST_Itr itr;
	proc_proxy_t *proxy_proc;

	MUTEX_LOCK(g_prox_proc_lock);
	for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
	{
		proxy_proc = *itr;
		if (proxy_proc->is_injected)
		{
			cnt++;
		}
	}
	MUTEX_UNLOCK(g_prox_proc_lock);
	return cnt;
}

int proxy_proc_add(char *proc_name, uint64_t proc_id)
{
	proc_proxy_t *proxy_proc = new proc_proxy_t;
	memset(proxy_proc, 0, sizeof(proc_proxy_t));

	strncpy(proxy_proc->proc_name, proc_name, PROC_NAME_LEN);
	proxy_proc->proc_name[PROC_NAME_LEN-1] = 0;

	proxy_proc->proc_id = proc_id;
	proxy_proc->status = PROC_ST_INIT;
	HANDLE proc_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)proc_id);
	if (proc_handle != NULL)
	{
		proxy_proc->is_proc_64 = is_process_64(proc_handle);
	}
	else
	{
		proxy_proc->is_proc_64 = false;
	}
    ::CloseHandle(proc_handle);

	MUTEX_LOCK(g_prox_proc_lock);
    g_prox_proc_objs.push_back(proxy_proc);
    MUTEX_UNLOCK(g_prox_proc_lock);

    _LOG_INFO(_T("add proxy process: %s:%d"), proc_name,proc_id);

    /*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
	return 0;
}

void proxy_proc_del(uint64_t proc_id)
{
	PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    MUTEX_LOCK(g_prox_proc_lock);
    for (itr = g_prox_proc_objs.begin(); itr != g_prox_proc_objs.end(); itr++)
    {
        proxy_proc = *itr;

        if (proxy_proc->proc_id == proc_id)
        {
        	g_prox_proc_objs.remove(proxy_proc);
        	MUTEX_UNLOCK(g_prox_proc_lock);

            if (proxy_proc->is_injected)
            {
                unInject_dll(proxy_proc->proc_name, proxy_proc->proc_id);
                proxy_proc->status = PROC_ST_UNINJECTED;
                proxy_proc->is_injected = false;
            }

        	_LOG_INFO(_T("del proxy process: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
        	delete proxy_proc;

        	MUTEX_UNLOCK(g_prox_proc_lock);
        	return;
        }
    }
    MUTEX_UNLOCK(g_prox_proc_lock);

    /*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
    return;
}

static void proxy_proc_del_aged(uint64_t aged_time)
{
	PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;

    MUTEX_LOCK(g_prox_proc_lock);

    for (itr = g_prox_proc_objs.begin();
            itr != g_prox_proc_objs.end();
            )
    {
        proxy_proc = *itr;

        if (false == is_proc_exist(proxy_proc->proc_name, proxy_proc->proc_id))
        {
            _LOG_INFO(_T("proxy process: %s:%d not exist, del it"), proxy_proc->proc_name, proxy_proc->proc_id);
            delete proxy_proc;
            itr = g_prox_proc_objs.erase(itr);
            continue;
        }

        if (proxy_proc->status == PROC_ST_INIT)
        {
            /*正在注入,可能时间比较长，不老化*/
            itr++;
            continue;
        }

		if (proxy_proc->update_time < aged_time)
        {
            if(proxy_proc->is_injected)
            {
				unInject_dll(proxy_proc->proc_name, proxy_proc->proc_id);
                proxy_proc->status = PROC_ST_UNINJECTED;
                proxy_proc->is_injected = false;
            }
	    	
			_LOG_INFO(_T("aged proxy process: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
	    	delete proxy_proc;
	        itr = g_prox_proc_objs.erase(itr);
   		}
   		else
   		{
   			itr++;
   		}
    }

    MUTEX_UNLOCK(g_prox_proc_lock);

    /*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
    return;
}

static HWND g_progDlgWnd = NULL;
static XPProgDlg *g_progDlg = NULL;

void proxy_proc_del_all()
{
	PROC_LIST_Itr itr;
    proc_proxy_t *proxy_proc;
    int ii = 0;

    if (g_progDlg != NULL)
    {
        g_progDlgWnd = g_progDlg->m_hWnd;
    }

    MUTEX_LOCK(g_prox_proc_lock);

    for (itr = g_prox_proc_objs.begin();
            itr != g_prox_proc_objs.end();
            )
    {
    	proxy_proc = *itr;

        if (proxy_proc->is_injected)
        {
            ii++;
            char *tipStr = new char[256];
            memset(tipStr, 0, 256);
            _snprintf_s(tipStr, 256, _TRUNCATE, _T("正在停止代理: %s"), proxy_proc->proc_name);

            /*通知窗口更新*/
            if (g_progDlgWnd != NULL)
				::PostMessage(g_progDlgWnd, WM_UPDATE_PROGRESS, (WPARAM)ii, (LPARAM)tipStr);

			unInject_dll(proxy_proc->proc_name, proxy_proc->proc_id);
    	   proxy_proc->status = PROC_ST_UNINJECTED;
           proxy_proc->is_injected = false;
        }

		_LOG_INFO(_T("destroy this, del proxy process: %s:%d"), proxy_proc->proc_name, proxy_proc->proc_id);
    	delete proxy_proc;

        itr = g_prox_proc_objs.erase(itr);        
    }

    MUTEX_UNLOCK(g_prox_proc_lock);
	
    /*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);

	/*通知窗口更新*/
	if (g_progDlgWnd != NULL)
    {
		::PostMessage(g_progDlgWnd, WM_UPDATE_PROGRESS, (WPARAM)0, NULL);
        g_progDlgWnd = NULL;
    }
    return;
}

void proxy_proc_update_all()
{
	static bool is_updating = false;

	MUTEX_LOCK(g_prox_proc_lock);
	if (is_updating == false)
	{
		is_updating = true;
	}
	else
	{
		MUTEX_UNLOCK(g_prox_proc_lock);
		return;
	}	
	MUTEX_UNLOCK(g_prox_proc_lock);

    if (g_is_start == false)
    {
        return;
    }
    
	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		uint64_t pid_arr[PROXY_CFG_MAX_PROC];
		int pid_cnt = get_proc_id_by_name(g_proxy_procs[ii], pid_arr);

        /*获取所有配置的进程相关的pid*/
		for (int jj = 0; jj < pid_cnt; jj++)
		{
			if (proxy_proc_is_exist(pid_arr[jj]) == false)
			{
				proxy_proc_add(g_proxy_procs[ii], pid_arr[jj]);
			}	
			else
			{
				proxy_proc_update(pid_arr[jj]);
			}		
		}
	}

	uint64_t nowTime = util_get_cur_time();
	proxy_proc_del_aged(nowTime - PROC_UPDATE_TIME*3);

	MUTEX_LOCK(g_prox_proc_lock);
	is_updating = false;
	MUTEX_UNLOCK(g_prox_proc_lock);

    /*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_PROC_LIST, NULL, NULL);
	return;
}

static void _proxy_update_timer_callback(void* param1, void* param2,
                void* param3, void* param4)
{
	proxy_proc_update_all();
}

int proxy_proc_mgr_init()
{
    MUTEX_SETUP(g_prox_proc_lock);
    return 0;
}

void proxy_proc_mgr_free()
{
	MUTEX_CLEANUP(g_prox_proc_lock);	
}

int  proxy_proc_mgr_start()
{
	/*start check timer*/
	np_add_time_job(_proxy_update_timer_callback, NULL, NULL, NULL, NULL, PROC_UPDATE_TIME, FALSE);
	return 0;
}

unsigned int WINAPI threadFunc(void * arg)
{
	np_del_time_job(_proxy_update_timer_callback, NULL);
	proxy_proc_del_all();
	return 0;
}

void proxy_proc_mgr_stop()
{
	/*获取进程个数*/
	int proc_cnt = proxy_proc_get_injected_cnt();
	if (proc_cnt == 0)
	{
		threadFunc(NULL);
		return;
	}

	HANDLE thrd_handle;
	thrd_handle = (HANDLE)_beginthreadex(NULL, 0, threadFunc, (void*)1, 0, NULL);
	
	CString tmpStr;
	tmpStr.Format(_T("正在停止"));
	XPProgDlg progDlg(proc_cnt, tmpStr);
	g_progDlg = &progDlg;
	progDlg.DoModal();
    g_progDlg = NULL;
    
	::WaitForSingleObject(thrd_handle, INFINITE);
	/*CloseHandle 只是减少了一个索引，线程真正退出时会释放资源*/
	::CloseHandle(thrd_handle);
}
