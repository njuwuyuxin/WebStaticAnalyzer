#include "framework/BasicChecker.h"
#include "clang/AST/Expr.h"
#include <cstdint>
#include <memory>

using namespace clang;

class Analyzer {
public:
  void bindZeroChecker(BasicChecker *checker) { zeroChecker = checker; }
  void bindCharArrayChecker(BasicChecker *checker) {
    charArrayChecker = checker;
  }

  void printValueList() {
    for (auto i : ValueList) {
      std::cout << "possible value of :" << i.var->getNameAsString()
                << std::endl;
      for (auto j : i.PosValue) {
        std::cout << j << " ";
      }
      if (i.var_type == V_CHAR_ARRAY) {
        auto values = std::static_pointer_cast<strSet>(i.values);
        for (auto &&s : *values) {
          std::cout << s << " ";
        }
      }
      std::cout << std::endl;
    }
  }

  void DealStmt(Stmt *stmt);
  void DealVarDecl(VarDecl *var);
  void DealCompoundStmt(Stmt *stmt);
  void DealIfStmt(IfStmt *ist);
  void DealSwitchStmt(SwitchStmt *sst);
  void DealForStmt(ForStmt *fst);
  void DealWhileStmt(WhileStmt *wst);
  void DealDoStmt(DoStmt *dst);

private:
  using uintSet = std::set<uint64_t>;
  using strSet = std::set<std::string>;

  enum { ERROR, WARNING };
  enum VarType {
    V_INT,
    V_UNSIGNED_INT,
    V_FLOAT,
    V_UNSIGNED_FLOAT,
    V_CHAR,
    V_CHAR_ARRAY,
    UNKNOWN
  };

  struct VarValue {
    VarDecl *var = nullptr;
    VarType var_type = UNKNOWN;
    bool isDefined = false;
    std::set<float> PosValue;
    std::shared_ptr<void> values;
  };

  std::vector<VarValue> ValueList;
  BasicChecker *zeroChecker = nullptr;
  BasicChecker *charArrayChecker = nullptr;

  void report(BinaryOperator *E, const VarValue &var) {
    if (zeroChecker != nullptr) {
      zeroChecker->report(E, var.PosValue.size() > 1 ? WARNING : ERROR);
    }
  }

  void report(ArraySubscriptExpr *E, const VarValue &var) {
    if (charArrayChecker != nullptr) {
      charArrayChecker->report(
          E, std::static_pointer_cast<strSet>(var.values)->size() > 1 ? WARNING
                                                                      : ERROR);
    }
  }

  bool judgeConsist(VarValue v1, VarValue v2) {
    if (v2.PosValue.size() == 0) {
      return false;
    }
    for (auto i : v2.PosValue) {
      if (v1.PosValue.find(i) == v1.PosValue.end()) {
        return false;
      }
    }
    return true;
  }

  int FindVarInList(VarDecl *waitToFind) {
    if (waitToFind == nullptr)
      return -1;
    for (int i = 0; i < ValueList.size(); i++) {
      if (waitToFind->getID() ==
          ValueList[i].var->getID()) {
        return i;
      }
    }
    return -1;
  }

  VarValue DealRValExpr(Expr *expr);
  VarValue DealBinaryOperator(BinaryOperator *E);
  VarValue DealUnaryOperator(UnaryOperator *E);
  VarValue DealConditionalOperator(ConditionalOperator *E);
  VarValue DealDivOp(VarValue v1, VarValue v2, BinaryOperator *E);
  VarValue DealModOp(VarValue v1, VarValue v2, BinaryOperator *E);
  VarValue DealArraySubscriptExpr(ArraySubscriptExpr *expr);

  VarValue DealAddOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i + j);
      }
    }
    return v3;
  }
  VarValue DealSubOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i - j);
      }
    }
    return v3;
  }
  VarValue DealMulOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i * j);
      }
    }
    return v3;
  }

  VarValue DealLogAndOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i && j));
      }
    }
    return v3;
  }
  VarValue DealLogOrOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i || j));
      }
    }
    return v3;
  }

  VarValue DealGTOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i > j));
      }
    }
    return v3;
  }
  VarValue DealGEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i >= j));
      }
    }
    return v3;
  }
  VarValue DealLTOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i < j));
      }
    }
    return v3;
  }
  VarValue DealLEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i <= j));
      }
    }
    return v3;
  }
  VarValue DealEQOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i == j));
      }
    }
    return v3;
  }
  VarValue DealNEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i != j));
      }
    }
    return v3;
  }

  VarValue DealPostInc(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPostDec(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPreInc(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPreDec(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPlus(VarValue v) { return v; }
  VarValue DealMinus(VarValue v) {
    VarValue PosResult;
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(-i);
    }
    return PosResult;
  }
};
