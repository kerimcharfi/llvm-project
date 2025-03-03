//===-- SwiftPersistentExpressionState.h ------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_SwiftPersistentExpressionState_h_
#define liblldb_SwiftPersistentExpressionState_h_

#include "SwiftExpressionVariable.h"

#include "swift/AST/Import.h"
#include "swift/AST/Module.h"

#include "lldb/Core/SwiftForward.h"
#include "lldb/Expression/ExpressionVariable.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <set>
#include <string>
#include <unordered_map>

namespace lldb_private {

/// Manages persistent values that need to be preserved between
/// expression invocations.
///
/// A list of variables that can be accessed and updated by any
/// expression.  See \ref ClangPersistentVariable for more discussion.
/// Also provides an increasing, 0-based counter for naming result
/// variables.
class SwiftPersistentExpressionState : public PersistentExpressionState {

  typedef llvm::StringMap<swift::AttributedImport<swift::ImportedModule>>
      HandLoadedModuleSet;

public:
  class SwiftDeclMap {
  public:
    void AddDecl(swift::ValueDecl *decl, bool check_existing, ConstString name);

    /// Find decls matching `name`, excluding decls that are equivalent to
    /// decls in `excluding_equivalents`, and put the results in `matches`.
    /// Return true if there are any results.
    bool FindMatchingDecls(
        ConstString name,
        const std::vector<swift::ValueDecl *> &excluding_equivalents,
        std::vector<swift::ValueDecl *> &matches);

    void CopyDeclsTo(SwiftDeclMap &target_map);
    static bool DeclsAreEquivalent(swift::Decl *lhs, swift::Decl *rhs);

  private:
    typedef std::unordered_multimap<std::string, swift::ValueDecl *>
        SwiftDeclMapTy;
    typedef SwiftDeclMapTy::iterator iterator;
    SwiftDeclMapTy m_swift_decls;
  };

  //----------------------------------------------------------------------
  /// Constructor
  //----------------------------------------------------------------------
  SwiftPersistentExpressionState();

  ~SwiftPersistentExpressionState() {}

  //------------------------------------------------------------------
  // llvm casting support
  //------------------------------------------------------------------
  // LLVM RTTI Support
  static char ID;

  lldb::ExpressionVariableSP
  CreatePersistentVariable(const lldb::ValueObjectSP &valobj_sp) override;

  lldb::ExpressionVariableSP
  CreatePersistentVariable(ExecutionContextScope *exe_scope, ConstString name,
                           const CompilerType &compiler_type,
                           lldb::ByteOrder byte_order,
                           uint32_t addr_byte_size) override;

  llvm::StringRef GetPersistentVariablePrefix(bool is_error) const override {
    return is_error ? "$E" : "$R";
  }

  void RemovePersistentVariable(lldb::ExpressionVariableSP variable) override;

  ConstString GetNextPersistentVariableName(bool is_error = false) override;

  llvm::Optional<CompilerType>
  GetCompilerTypeFromPersistentDecl(ConstString type_name) override;

  void RegisterSwiftPersistentDecl(swift::ValueDecl *value_decl);

  void RegisterSwiftPersistentDeclAlias(swift::ValueDecl *value_decl,
                                        ConstString name);

  void CopyInSwiftPersistentDecls(SwiftDeclMap &source_map);

  /// Find decls matching `name`, excluding decls that are equivalent to decls
  /// in `excluding_equivalents`, and put the results in `matches`.  Return true
  /// if there are any results.
  bool GetSwiftPersistentDecls(
      ConstString name,
      const std::vector<swift::ValueDecl *> &excluding_equivalents,
      std::vector<swift::ValueDecl *> &matches);

  // This just adds this module to the list of hand-loaded modules, it doesn't
  // actually load it.
  void AddHandLoadedModule(
      ConstString module_name,
      swift::AttributedImport<swift::ImportedModule> attributed_import) {
    m_hand_loaded_modules.insert_or_assign(module_name.GetStringRef(),
                                           attributed_import);
  }

  /// This returns the list of hand-loaded modules.
  HandLoadedModuleSet GetHandLoadedModules() { return m_hand_loaded_modules; }

private:
  /// The counter used by GetNextResultName().
  uint32_t m_next_persistent_variable_id;
  /// The counter used by GetNextResultName() when is_error is true.
  uint32_t m_next_persistent_error_id;
  /// The persistent functions declared by the user.
  SwiftDeclMap m_swift_persistent_decls;
  /// These are the names of modules that we have loaded by hand into
  /// the Contexts we make for parsing.
  HandLoadedModuleSet m_hand_loaded_modules;
};
} // namespace lldb_private

#endif
