#include "framework/BasicChecker.h"

using namespace clang;
using namespace std;

class Analyzer {
public:
  void bindZeroChecker(BasicChecker *checker) { zeroChecker = checker; }

  void printValueList() {
    for (auto i : ValueList) {
      cout << "possible value of :" << i.var->getNameAsString() << endl;
      for (auto j : i.PosValue) {
        cout << j << " ";
      }
      cout << endl;
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
  enum { ERROR, WARNING };

  enum VarType { V_INT, V_UNSIGNED_INT, V_FLOAT, V_UNSIGNED_FLOAT, UNKNOWN };

  struct VarValue {
    VarDecl *var;
    VarType var_type;
    bool isDefined;
    set<float> PosValue;
    VarValue() {
      var = NULL;
      var_type = UNKNOWN;
      isDefined = false;
    }
  };

  vector<VarValue> ValueList;
  BasicChecker *zeroChecker = nullptr;

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
      if (waitToFind->getNameAsString() ==
          ValueList[i].var->getNameAsString()) {
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
