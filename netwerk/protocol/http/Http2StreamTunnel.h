/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_net_Http2StreamTunnel_h
#define mozilla_net_Http2StreamTunnel_h

#include "Http2StreamBase.h"

namespace mozilla {
namespace net {

class Http2StreamTunnel : public Http2StreamBase {
 public:
  Http2StreamTunnel(nsAHttpTransaction* httpTransaction, Http2Session* session,
                    int32_t priority, uint64_t bcId)
      : Http2StreamBase(httpTransaction, session, priority, bcId) {}

  bool IsTunnel() override { return true; }

  nsCString& RegistrationKey();

 protected:
  ~Http2StreamTunnel();
  void HandleResponseHeaders(nsACString& aHeadersOut,
                             int32_t httpResponseCode) override;

 private:
  void ClearTransactionsBlockedOnTunnel();
  bool MapStreamToHttpConnection(const nsACString& aFlat407Headers,
                                 int32_t aHttpResponseCode);
  void MapStreamToPlainText();

  bool mPlainTextTunnel{false};
  nsCString mRegistrationKey;
};

}  // namespace net
}  // namespace mozilla

#endif  // mozilla_net_Http2StreamTunnel_h
