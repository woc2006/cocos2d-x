/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <string>
#include <vector>
#include <deque>
#include "base/CCRef.h"
#include "base/ccMacros.h"

NS_CC_BEGIN

namespace network {

class HttpClient;
class HttpResponse;

typedef std::function<void(HttpClient* client, HttpResponse* response)> ccHttpRequestCallback;
typedef void (cocos2d::Ref::*SEL_HttpResponse)(HttpClient* client, HttpResponse* response);
#define httpresponse_selector(_SELECTOR) (cocos2d::network::SEL_HttpResponse)(&_SELECTOR)

/**
 @brief defines the object which users must packed for HttpClient::send(HttpRequest*) method.
 Please refer to tests/test-cpp/Classes/ExtensionTest/NetworkTest/HttpClientTest.cpp as a sample
 @since v2.0.2
 */

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
#ifdef DELETE
#undef DELETE
#endif
#endif

class HttpRequest : public Ref
{
public:
    /** Use this enum type as param in setReqeustType(param) */
    enum class Type
    {
        GET,
        POST,
        PUT,
        DELETE,
		DOWNLOAD,
		UPLOAD,
        VISIT,  //try visit server, return connection time
        UNKNOWN,
    };

    /** Constructor
        Because HttpRequest object will be used between UI thead and network thread,
        requestObj->autorelease() is forbidden to avoid crashes in AutoreleasePool
        new/retain/release still works, which means you need to release it manually
        Please refer to HttpRequestTest.cpp to find its usage
     */
    HttpRequest()
    {
        _requestType = Type::UNKNOWN;
        _url.clear();
        _requestData.clear();
        _tag.clear();
        _pTarget = nullptr;
        _pSelector = nullptr;
        _pCallback = nullptr;
        _pUserData = nullptr;
    };

    /** Destructor */
    virtual ~HttpRequest()
    {
        if (_pTarget)
        {
            _pTarget->release();
        }
        this->closeAllFiles();
    };

    /** Override autorelease method to avoid developers to call it */
    Ref* autorelease(void)
    {
        CCASSERT(false, "HttpResponse is used between network thread and ui thread \
                 therefore, autorelease is forbidden here");
        return NULL;
    }

    // setter/getters for properties

    /** Required field for HttpRequest object before being sent.
        kHttpGet & kHttpPost is currently supported
     */
    inline void setRequestType(Type type)
    {
        _requestType = type;
    };
    /** Get back the kHttpGet/Post/... enum value */
    inline Type getRequestType()
    {
        return _requestType;
    };

    /** Required field for HttpRequest object before being sent.
     */
    inline void setUrl(const char* url)
    {
        _url = url;
    };
    /** Get back the setted url */
    inline const char* getUrl()
    {
        return _url.c_str();
    };

    inline void addFileTarget(const char* path, FILE* fp){
        std::string target(path);
        auto it = _filePacks.find(path);
        if(it != _filePacks.end()){
            if(it->second != nullptr){
                ::fclose(it->second);
            }
            it->second = fp;
        }else{
            _filePacks.insert(std::make_pair(target,fp));
        }
    }

    inline void closeFile(const char* path){
        auto it = _filePacks.find(path);
        if(it!=_filePacks.end()){
            if(it->second != nullptr){
                ::fclose(it->second);
                it->second = nullptr;
            }
        }
    }

    inline void closeAllFiles(){
        for(auto& f : _filePacks){
            if(f.second != nullptr){
                ::fclose(f.second);
                f.second = nullptr;
            }
        }
       // _filePacks.clear();
    }

    inline const std::map<std::string, FILE*>& getTargetFileList()const{
        return _filePacks;
    }



    /* this part is not used anymore, multiple file target is supported now
    /* As soon as the filename is set, the file pointer will be closed and set to null
	inline void setFilename(const char* path)
	{
		_filename = path;
        if(this->pFile != nullptr){
            ::fclose(this->pFile);
            this->pFile = nullptr;
        }
	};
	/** Get back the setted url
	inline const char* getFilename()
	{
		return _filename.c_str();
	};

    /* As soon as valid fp is set, it will be used instead of the _filename. The passing fp may not be closed out side the request. it will be closed by the request itself
    inline void setFilePointer(FILE* fp){
		if(pFile != nullptr && pFile != fp){
			closeFile();
		}
        if(fp!=nullptr){
            this->pFile = fp;
        }
    }

    inline void closeFile(void){
        if(this->pFile != nullptr){
            ::fclose(this->pFile);
            this->pFile = nullptr;
        }
    }

    inline FILE * getFilePointer(void){
        return this->pFile;
    }
    */
    /** Option field. You can set your post data here
     */
    inline void setRequestData(const char* buffer, size_t len)
    {
        _requestData.assign(buffer, buffer + len);
    };
    /** Get the request data pointer back */
    inline char* getRequestData()
    {
        if(_requestData.size() != 0)
            return &(_requestData.front());

        return nullptr;
    }
    /** Get the size of request data back */
    inline ssize_t getRequestDataSize()
    {
        return _requestData.size();
    }

    /** Option field. You can set a string tag to identify your request, this tag can be found in HttpResponse->getHttpRequest->getTag()
     */
    inline void setTag(const char* tag)
    {
        _tag = tag;
    };

	inline void setIntTag(int tag)
	{
		_inttag = tag;
	};
    /** Get the string tag back to identify the request.
        The best practice is to use it in your MyClass::onMyHttpRequestCompleted(sender, HttpResponse*) callback
     */
    inline const char* getTag()
    {
        return _tag.c_str();
    };

	inline int getIntTag()
	{
		return _inttag;
	};

    /** Option field. You can attach a customed data in each request, and get it back in response callback.
        But you need to new/delete the data pointer manully
     */
    inline void setUserData(void* pUserData)
    {
        _pUserData = pUserData;
    };
    /** Get the pre-setted custom data pointer back.
        Don't forget to delete it. HttpClient/HttpResponse/HttpRequest will do nothing with this pointer
     */
    inline void* getUserData()
    {
        return _pUserData;
    };

    /** Required field. You should set the callback selector function at ack the http request completed
     */
    CC_DEPRECATED_ATTRIBUTE inline void setResponseCallback(Ref* pTarget, SEL_CallFuncND pSelector)
    {
        setResponseCallback(pTarget, (SEL_HttpResponse) pSelector);
    }

    CC_DEPRECATED_ATTRIBUTE inline void setResponseCallback(Ref* pTarget, SEL_HttpResponse pSelector)
    {
        _pTarget = pTarget;
        _pSelector = pSelector;

        if (_pTarget)
        {
            _pTarget->retain();
        }
    }

    inline void setResponseCallback(const ccHttpRequestCallback& callback)
    {
        _pCallback = callback;
    }

    /** Get the target of callback selector funtion, mainly used by HttpClient */
    inline Ref* getTarget()
    {
        return _pTarget;
    }

    /* This sub class is just for migration SEL_CallFuncND to SEL_HttpResponse,
       someday this way will be removed */
    class _prxy
    {
    public:
        _prxy( SEL_HttpResponse cb ) :_cb(cb) {}
        ~_prxy(){};
        operator SEL_HttpResponse() const { return _cb; }
        CC_DEPRECATED_ATTRIBUTE operator SEL_CallFuncND()   const { return (SEL_CallFuncND) _cb; }
    protected:
        SEL_HttpResponse _cb;
    };

    /** Get the selector function pointer, mainly used by HttpClient */
    inline _prxy getSelector()
    {
        return _prxy(_pSelector);
    }

    inline const ccHttpRequestCallback& getCallback()
    {
        return _pCallback;
    }

    /** Set any custom headers **/
    inline void setHeaders(std::vector<std::string> pHeaders)
   	{
   		_headers=pHeaders;
   	}

    inline void addHeader(string header){
        _headers.push_back(header);
    }

    /** Get custom headers **/
   	inline std::vector<std::string> getHeaders()
   	{
   		return _headers;
   	}

protected:

    // properties
    Type                        _requestType;    /// kHttpRequestGet, kHttpRequestPost or other enums
    std::string                 _url;            /// target url that this request is sent to
	//std::string				_filename;
    //FILE*                   pFile;               /// A File Pointer to the Target. pFile will be closed and set to null when _filename is assigned.
    std::map<std::string,FILE*> _filePacks;
    std::vector<char>           _requestData;    /// used for POST
    std::string                 _tag;            /// user defined tag, to identify different requests in response callback
	int							_inttag;
    Ref*                        _pTarget;        /// callback target of pSelector function
    SEL_HttpResponse            _pSelector;      /// callback function, e.g. MyLayer::onHttpResponse(HttpClient *sender, HttpResponse * response)
    ccHttpRequestCallback       _pCallback;      /// C++11 style callbacks
    void*                       _pUserData;      /// You can add your customed data here
    std::vector<std::string>    _headers;		      /// custom http headers
};

}

NS_CC_END

#endif //__HTTP_REQUEST_H__
