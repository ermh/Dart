// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library("http_parser_test.dart");
#import("../../../chat/http.dart");


class HTTPParserTest {
  static void runAllTests() {
    testParseRequest();
    testParseResponse();
  }

  static void _testParseRequest(String request,
                                String expectedMethod,
                                String expectedUri,
                                [int expectedContentLength = 0,
                                 int expectedBytesReceived = 0,
                                 Map expectedHeaders = null,
                                 bool chunked = false]) {
    HTTPParser httpParser;
    bool headersCompleteCalled;
    bool dataEndCalled;
    String method;
    String uri;
    Map headers;
    int contentLength;
    int bytesReceived;

    void reset() {
      httpParser = new HTTPParser();
      httpParser.requestStart = (m, u) { method = m; uri = u; };
      httpParser.responseStart = (s, r) { Expect.fail("Expected request"); };
      httpParser.headerReceived =
          (f, v) {
            Expect.isFalse(headersCompleteCalled);
            headers[f] = v;
          };
      httpParser.headersComplete =
          () {
            Expect.isFalse(headersCompleteCalled);
            if (!chunked) {
              Expect.equals(expectedContentLength, httpParser.contentLength);
            } else {
              Expect.equals(-1, httpParser.contentLength);
            }
            if (expectedHeaders != null) {
              expectedHeaders.forEach(
                  (String name, String value) =>
                      Expect.equals(value, headers[name]));
            }
            headersCompleteCalled = true;
          };
      httpParser.dataReceived =
          (List<int> data) {
            Expect.isTrue(headersCompleteCalled);
            bytesReceived += data.length;
          };
      httpParser.dataEnd = () { dataEndCalled = true; };

      headersCompleteCalled = false;
      dataEndCalled = false;
      method = null;
      uri = null;
      headers = new Map();
      bytesReceived = 0;
    }

    void checkExpectations() {
      Expect.equals(expectedMethod, method);
      Expect.equals(expectedUri, uri);
      Expect.isTrue(headersCompleteCalled);
      Expect.equals(expectedBytesReceived, bytesReceived);
      Expect.isTrue(dataEndCalled);
    }

    void testWrite(List<int> requestData, [int chunkSize = -1]) {
      if (chunkSize == -1) chunkSize = requestData.length;
      reset();
      for (int pos = 0; pos < requestData.length; pos += chunkSize) {
        int remaining = requestData.length - pos;
        int writeLength = Math.min(chunkSize, remaining);
        httpParser.writeList(requestData, pos, writeLength);
      }
      checkExpectations();
    }

    void testWriteAll(List<int> requestData) {
      reset();
      httpParser.writeList(requestData, 0, requestData.length);
      checkExpectations();
    }

    // Test parsing the request three times delivering the data in
    // different chunks.
    List<int> requestData = request.charCodes();
    testWrite(requestData);
    testWrite(requestData, 10);
    testWrite(requestData, 1);
  }

  static void _testParseResponse(String response,
                                 int expectedStatusCode,
                                 String expectedReasonPhrase,
                                 [int expectedContentLength = 0,
                                  int expectedBytesReceived = 0,
                                  Map expectedHeaders = null,
                                  bool chunked = false]) {
    HTTPParser httpParser;
    bool headersCompleteCalled;
    bool dataEndCalled;
    int statusCode;
    String reasonPhrase;
    Map headers;
    int contentLength;
    int bytesReceived;

    void reset() {
      httpParser = new HTTPParser();
      httpParser.requestStart = (m, u) { Expect.fail("Expected response"); };
      httpParser.responseStart = (s, r) { statusCode = s; reasonPhrase = r; };
      httpParser.headerReceived =
          (f, v) {
            Expect.isFalse(headersCompleteCalled);
            headers[f] = v;
          };
      httpParser.headersComplete =
          () {
            Expect.isFalse(headersCompleteCalled);
            if (!chunked) {
              Expect.equals(expectedContentLength, httpParser.contentLength);
            } else {
              Expect.equals(-1, httpParser.contentLength);
            }
            if (expectedHeaders != null) {
              expectedHeaders.forEach(
                  (String name, String value) =>
                      Expect.equals(value, headers[name]));
            }
            headersCompleteCalled = true;
          };
      httpParser.dataReceived =
          (List<int> data) {
            Expect.isTrue(headersCompleteCalled);
            bytesReceived += data.length;
          };
      httpParser.dataEnd = () { dataEndCalled = true; };

      headersCompleteCalled = false;
      dataEndCalled = false;
      statusCode = -1;
      reasonPhrase = null;
      headers = new Map();
      bytesReceived = 0;
    }

    void checkExpectations() {
      Expect.equals(expectedStatusCode, statusCode);
      Expect.equals(expectedReasonPhrase, reasonPhrase);
      Expect.isTrue(headersCompleteCalled);
      Expect.equals(expectedBytesReceived, bytesReceived);
      Expect.isTrue(dataEndCalled);
    }

    void testWrite(List<int> requestData, [int chunkSize = -1]) {
      if (chunkSize == -1) chunkSize = requestData.length;
      reset();
      for (int pos = 0; pos < requestData.length; pos += chunkSize) {
        int remaining = requestData.length - pos;
        int writeLength = Math.min(chunkSize, remaining);
        httpParser.writeList(requestData, pos, writeLength);
      }
      checkExpectations();
    }

    void testWriteAll(List<int> requestData) {
      reset();
      httpParser.writeList(requestData, 0, requestData.length);
      checkExpectations();
    }

    // Test parsing the request three times delivering the data in
    // different chunks.
    List<int> responseData = response.charCodes();
    testWrite(responseData);
    testWrite(responseData, 10);
    testWrite(responseData, 1);
  }

  static void testParseRequest() {
    String request;
    Map headers;
    request = "GET / HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "GET", "/");

    request = "POST / HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "POST", "/");

    request = "GET /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "GET", "/index.html");

    request = "POST /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "POST", "/index.html");

    request = "H /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "H", "/index.html");

    request = "HT /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "HT", "/index.html");

    request = "HTT /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "HTT", "/index.html");

    request = "HTTP /index.html HTTP/1.1\r\n\r\n";
    _testParseRequest(request, "HTTP", "/index.html");

    request = """
POST /test HTTP/1.1\r
AAA: AAA\r
Content-Length: 0\r
\r
""";
    _testParseRequest(request, "POST", "/test");

    request = """
POST /test HTTP/1.1\r
content-length: 0\r
\r
""";
    _testParseRequest(request, "POST", "/test");

    request = """
POST /test HTTP/1.1\r
Header-A: AAA\r
X-Header-B: bbb\r
\r
""";
    headers = new Map();
    headers["header-a"] = "AAA";
    headers["x-header-b"] = "bbb";
    _testParseRequest(request, "POST", "/test", 0, 0, headers);

    request = """
POST /test HTTP/1.1\r
Header-A:   AAA\r
X-Header-B:\t \t bbb\r
\r
""";
    headers = new Map();
    headers["header-a"] = "AAA";
    headers["x-header-b"] = "bbb";
    _testParseRequest(request, "POST", "/test", 0, 0, headers);

    request = """
POST /test HTTP/1.1\r
Header-A:   AA\r
 A\r
X-Header-B:           b\r
  b\r
\t    b\r
\r
""";
    headers = new Map();
    headers["header-a"] = "AAA";
    headers["x-header-b"] = "bbb";
    _testParseRequest(request, "POST", "/test", 0, 0, headers);

    request = """
POST /test HTTP/1.1\r
Content-Length: 10\r
\r
0123456789""";
    _testParseRequest(request, "POST", "/test", 10, 10);

    // Test chunked encoding.
    request = """
POST /test HTTP/1.1\r
Transfer-Encoding: chunked\r
\r
5\r
01234\r
5\r
56789\r
0\r\n""";
    _testParseRequest(request, "POST", "/test", -1, 10, null, true);

    // Test mixing chunked encoding and content length (content length
    // is ignored).
    request = """
POST /test HTTP/1.1\r
Content-Length: 7\r
Transfer-Encoding: chunked\r
\r
5\r
01234\r
5\r
56789\r
0\r\n""";
    _testParseRequest(request, "POST", "/test", -1, 10, null, true);

    // Test mixing chunked encoding and content length (content length
    // is ignored).
    request = """
POST /test HTTP/1.1\r
Transfer-Encoding: chunked\r
Content-Length: 3\r
\r
5\r
01234\r
5\r
56789\r
0\r\n""";
    _testParseRequest(request, "POST", "/test", -1, 10, null, true);

    // Test upper and lower case hex digits in chunked encoding.
    request = """
POST /test HTTP/1.1\r
Transfer-Encoding: chunked\r
\r
1E\r
012345678901234567890123456789\r
1e\r
012345678901234567890123456789\r
0\r\n""";
    _testParseRequest(request, "POST", "/test", -1, 60, null, true);
  }

  static void testParseResponse() {
    String response;
    Map headers;
    response = "HTTP/1.1 200 OK\r\n\r\n";
    _testParseResponse(response, 200, "OK");

    response = "HTTP/1.1 404 Not found\r\n\r\n";
    _testParseResponse(response, 404, "Not found");

    response = "HTTP/1.1 500 Server error\r\n\r\n";
    _testParseResponse(response, 500, "Server error");

    // Test content.
    response = """
HTTP/1.1 200 OK\r
Content-Length: 20\r
\r
01234567890123456789""";

    _testParseResponse(response, 200, "OK", 20, 20);
    // Test upper and lower case hex digits in chunked encoding.
    response = """
HTTP/1.1 200 OK\r
Transfer-Encoding: chunked\r
\r
1A\r
01234567890123456789012345\r
1f\r
0123456789012345678901234567890\r
0\r\n""";
    _testParseResponse(response, 200, "OK", -1, 57, null, true);
  }
}


void main() {
  HTTPParserTest.runAllTests();
}
