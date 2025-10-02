# EuroScope Auth Injector

EuroScope身份认证注入器

## 注入原理

EuroScope使用`libcurl`开源库进行HTTP(s)访问  
本项目使用[Detours](https://github.com/microsoft/Detours)库  
对`libcurl`指定函数进行注入, 从而实现匹配目标url并替换为指定url  

## 如何使用

首先克隆本仓库  
随后进入`src/definition.h`文件  
将其中`INJECTOR_URL`定义的url改成你自己的url  
url请求参数和返回值示例如下

```http request
POST /api/users/sessions/fsd HTTP/1.1
Host: 127.0.0.1:6810
Accept: */*
Referer: EuroScope
User-Agent: EuroScope
Content-Type: application/json

{"cid":"2352","is_sweatbox":false,"password":"123456"}
HTTP/1.1 200 OK
Content-Type: application/json
Vary: Origin
Vary: Accept-Encoding
X-Content-Type-Options: nosniff
X-Frame-Options: SAMEORIGIN
X-Xss-Protection: 1; mode=block
Date: Thu, 02 Oct 2025 17:46:59 GMT
Content-Length: 347

{"success":true,"error_msg":"","token":"eyJhbGciOiJIUzUxMiIsInR5cCI6IkpXVCJ9.eyJjb250cm9sbGVyX3JhdGluZyI6MTIsInBpbG90X3JhdGluZyI6MCwiaXNzIjoiRnNkSHR0cFNlcnZlciIsInN1YiI6IkhhbGZfbm90aGluZyIsImV4cCI6MTc1OTQyODExOSwibmJmIjoxNzU5NDI3MjE5LCJpYXQiOjE3NTk0MjcyMTl9.7YZPp0PtAyB-9TNhmn0EFhVJbrM91O8q1HA-hcSGhJJtmqcZuKkRXLI1rsbXVjXVPPb0Ee_sKmb24oAY8je3Pw"}
```

修改完成之后编译, 在`bin`目录下会得到`EuroScopeInjector.dll`  
这就是编译完成的插件, 使用`EuroScope`加载即可使用

## 开源协议

MIT License

Copyright © 2025 Half_nothing

无附加条款。

## 行为准则

在[CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md)中查阅
