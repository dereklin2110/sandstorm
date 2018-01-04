// Sandstorm - Personal Cloud Sandbox
// Copyright (c) 2017 Sandstorm Development Group, Inc. and contributors
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SANDSTORM_WEB_SESSION_BRIDGE_H_
#define SANDSTORM_WEB_SESSION_BRIDGE_H_

#include <kj/compat/http.h>
#include <sandstorm/web-session.capnp.h>
#include "util.h"

namespace sandstorm {

class WebSessionBridge: public kj::HttpService, public kj::Refcounted {
public:
  class Tables {
    // Tables that many instances of WebSessionBridge might share. Create this object at startup
    // time and pass it to the constructor of each WebSessionBridge.

  public:
    Tables(kj::HttpHeaderTable::Builder& headerTableBuilder);

  private:
    friend class WebSessionBridge;

    const kj::HttpHeaderTable& headerTable;

    kj::HttpHeaderId hAccept;
    kj::HttpHeaderId hAcceptEncoding;
    kj::HttpHeaderId hContentDisposition;
    kj::HttpHeaderId hContentEncoding;
    kj::HttpHeaderId hContentLanguage;
    kj::HttpHeaderId hCookie;
    kj::HttpHeaderId hETag;
    kj::HttpHeaderId hIfMatch;
    kj::HttpHeaderId hIfNoneMatch;
    kj::HttpHeaderId hSecWebSocketProtocol;

    kj::Array<HttpStatusDescriptor::Reader> successCodeTable;
    kj::Array<HttpStatusDescriptor::Reader> errorCodeTable;
    HeaderWhitelist requestHeaderWhitelist;
    HeaderWhitelist responseHeaderWhitelist;
  };

  struct Options {
    bool allowCookies = false;
    // Should cookies be passed through or dropped on the floor?

    bool isHttps = false;
    // Will we be serving over HTTPS?
  };

  WebSessionBridge(WebSession::Client session, kj::Maybe<Handle::Client> loadingIndicator,
                   const Tables& tables, Options options);

  kj::Promise<void> request(
      kj::HttpMethod method, kj::StringPtr url, const kj::HttpHeaders& headers,
      kj::AsyncInputStream& requestBody, Response& response) override;

  kj::Promise<void> openWebSocket(
      kj::StringPtr url, const kj::HttpHeaders& headers, WebSocketResponse& response) override;

private:
  WebSession::Client session;
  kj::Maybe<Handle::Client> loadingIndicator;
  const Tables& tables;
  Options options;

  template <typename T>
  inline HttpStatusDescriptor::Reader lookupStatus(
      kj::ArrayPtr<const HttpStatusDescriptor::Reader> table,
      T codeEnum);

  kj::Promise<void> sendError(kj::HttpService::Response& response,
                              uint statusCode, kj::StringPtr statusText);

  struct ContextInitInfo {
    kj::Own<kj::PromiseFulfiller<ByteStream::Client>> streamer;
    bool hadIfNoneMatch = false;
  };

  ContextInitInfo initContext(
      WebSession::Context::Builder context, const kj::HttpHeaders& headers);

  template <typename Builder>
  void initContent(Builder&& builder, const kj::HttpHeaders& headers);

  capnp::Orphan<capnp::List<WebSession::ETag>> parseETagList(
      capnp::Orphanage orphanage, kj::StringPtr text,
      kj::Vector<kj::Tuple<kj::String, bool>> parsed = kj::Vector<kj::Tuple<kj::String, bool>>());

  kj::Tuple<kj::String, bool> parseETagInternal(kj::StringPtr& text);

  kj::Promise<void> handleStreamingRequestResponse(
      WebSession::RequestStream::Client reqStream,
      kj::AsyncInputStream& requestBody,
      ContextInitInfo&& contextInitInfo,
      kj::HttpService::Response& out);

  kj::Promise<void> handleResponse(kj::Promise<capnp::Response<WebSession::Response>>&& promise,
                                   ContextInitInfo&& contextInitInfo,
                                   kj::HttpService::Response& out);

  template <typename T>
  kj::Promise<void> handleErrorBody(T error, uint statusCode, kj::StringPtr statusText,
                                    kj::HttpHeaders& headers,
                                    capnp::Response<WebSession::Response>&& in,
                                    kj::HttpService::Response& out);

  void setETag(kj::HttpHeaders& headers, WebSession::ETag::Reader etag);

  kj::String escape(kj::StringPtr value);

  class ByteStreamImpl;
};

}  // namespace sandstorm

#endif  // SANDSTORM_WEB_SESSION_BRIDGE_H_
