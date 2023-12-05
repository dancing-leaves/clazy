/*
  This file is part of the clazy static checker.

  Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Waqar Ahmed <waqar.ahmed@kdab.com>

  Copyright (C) 2021 Waqar Ahmed <waqar.17a@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "use-arrow-operator-instead-of-data.h"

#include "HierarchyUtils.h"
#include <clang/AST/ExprCXX.h>

using namespace std;
using namespace clang;

UseArrowOperatorInsteadOfData::UseArrowOperatorInsteadOfData(const std::string &name, ClazyContext *context)
    : CheckBase(name, context)
{
}

void UseArrowOperatorInsteadOfData::VisitStmt(clang::Stmt *stmt)
{
    auto ce = dyn_cast<CXXMemberCallExpr>(stmt);
    if (!ce) {
        return;
    }

    auto vec = Utils::callListForChain(ce);
    if (vec.size() < 2) {
        return;
    }
    
    CallExpr *callExpr = vec.at(vec.size() - 1);

    FunctionDecl* funcDecl = callExpr->getDirectCallee();
    if (!funcDecl) {
        return;
    }
    const std::string func = clazy::qualifiedMethodName(funcDecl);

    static const std::vector<std::string> whiteList {
        "QScopedPointer::data",
        "QPointer::data",
        "QSharedPointer::data",
        "QSharedDataPointer::data"
    };

    bool accepted = clazy::any_of(whiteList, [func](const std::string& f) { return f == func; });
    if (!accepted) {
        return;
    }

    std::vector<FixItHint> fixits;

    constexpr int MinPossibleColonPos = sizeof("QPointer") - 1;
    const std::string ClassName = func.substr(0, func.find(':', MinPossibleColonPos));

    auto begin = callExpr->getExprLoc();
    const auto end = callExpr->getEndLoc();

    // find '.' in ptr.data()
    int dotOffset = 0;
    const char *d = m_sm.getCharacterData(begin);
    while (*d != '.') {
        dotOffset--;
        d--;
    }
    begin = begin.getLocWithOffset(dotOffset);

    const SourceRange sourceRange{begin, end};
    FixItHint removal = FixItHint::CreateRemoval(sourceRange);
    fixits.push_back(std::move(removal));

    emitWarning(clazy::getLocStart(callExpr), "Use operator -> directly instead of " + ClassName + "::data()->", fixits);
}
