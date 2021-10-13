/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/commands/plan_cache_commands.h"

#include "mongo/db/matcher/extensions_callback_real.h"

namespace mongo::plan_cache_commands {

StatusWith<std::unique_ptr<CanonicalQuery>> canonicalize(OperationContext* opCtx,
                                                         StringData ns,
                                                         const BSONObj& cmdObj) {
    // query - required
    BSONElement queryElt = cmdObj.getField("query");
    if (queryElt.eoo()) {
        return Status(ErrorCodes::BadValue, "required field query missing");
    }
    if (!queryElt.isABSONObj()) {
        return Status(ErrorCodes::BadValue, "required field query must be an object");
    }
    if (queryElt.eoo()) {
        return Status(ErrorCodes::BadValue, "required field query missing");
    }
    BSONObj queryObj = queryElt.Obj();

    // sort - optional
    BSONElement sortElt = cmdObj.getField("sort");
    BSONObj sortObj;
    if (!sortElt.eoo()) {
        if (!sortElt.isABSONObj()) {
            return Status(ErrorCodes::BadValue, "optional field sort must be an object");
        }
        sortObj = sortElt.Obj();
    }

    // projection - optional
    BSONElement projElt = cmdObj.getField("projection");
    BSONObj projObj;
    if (!projElt.eoo()) {
        if (!projElt.isABSONObj()) {
            return Status(ErrorCodes::BadValue, "optional field projection must be an object");
        }
        projObj = projElt.Obj();
    }

    // collation - optional
    BSONObj collationObj;
    if (auto collationElt = cmdObj["collation"]) {
        if (!collationElt.isABSONObj()) {
            return Status(ErrorCodes::BadValue, "optional field collation must be an object");
        }
        collationObj = collationElt.Obj();
        if (collationObj.isEmpty()) {
            return Status(ErrorCodes::BadValue,
                          "optional field collation cannot be an empty object");
        }
    }

    // Create canonical query
    const NamespaceString nss(ns);
    auto qr = std::make_unique<QueryRequest>(std::move(nss));
    qr->setFilter(queryObj);
    qr->setSort(sortObj);
    qr->setProj(projObj);
    qr->setCollation(collationObj);
    const ExtensionsCallbackReal extensionsCallback(opCtx, &nss);
    const boost::intrusive_ptr<ExpressionContext> expCtx;
    auto statusWithCQ =
        CanonicalQuery::canonicalize(opCtx,
                                     std::move(qr),
                                     expCtx,
                                     extensionsCallback,
                                     MatchExpressionParser::kAllowAllSpecialFeatures);
    if (!statusWithCQ.isOK()) {
        return statusWithCQ.getStatus();
    }

    return std::move(statusWithCQ.getValue());
}

}  // namespace mongo::plan_cache_commands
