# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Explicit, applicable C structural alternatives for small bounded searches.

Plans are enumerated on the current AST, so a missing site is a finite outcome.
These conservative source rewrites do not infer original types or prove a match.
The caller still owns source-method, ABI and whole-program review.
"""
from __future__ import annotations

import copy
from dataclasses import dataclass
from typing import Callable


PASSES = ('reverse_cond', 'reorder_uninitialized_decls', 'guard_return',
          'while_to_guarded_do')


@dataclass
class Plan:
    name: str
    apply: Callable[[], None]


def plans(ast, fn_name: str, ca, enabled, region=None) -> list[Plan]:
    """Enumerate transformations with sites; touch only the assigned function."""
    unknown = set(enabled) - set(PASSES)
    if unknown:
        raise ValueError('unknown structural pass: ' + ', '.join(sorted(unknown)))
    functions = [n for n in ast.ext
                 if isinstance(n, ca.FuncDef) and n.decl.name == fn_name]
    if len(functions) != 1:
        raise ValueError('expected one assigned function')
    fn = functions[0]
    result: list[Plan] = []
    address_taken = set()

    class AddressVisitor(ca.NodeVisitor):
        def visit_UnaryOp(self, node):
            if node.op == '&' and isinstance(node.expr, ca.ID):
                address_taken.add(node.expr.name)
            self.generic_visit(node)

    AddressVisitor().visit(fn.body)

    def inside(node):
        return region is None or region.contains_node(node)

    def has(node, kinds):
        if isinstance(node, kinds):
            return True
        return any(has(child, kinds) for _name, child in node.children())

    def terminal_return(node):
        return isinstance(node, ca.Return) or (
            isinstance(node, ca.Compound) and node.block_items
            and isinstance(node.block_items[-1], ca.Return))

    def defines_tag(node):
        if isinstance(node, (ca.Struct, ca.Union)) and node.decls is not None:
            return True
        if isinstance(node, ca.Enum) and node.values is not None:
            return True
        return any(defines_tag(child) for _key, child in node.children())

    def plain_decl(node):
        if (not isinstance(node, ca.Decl) or not node.name or node.init is not None
                or node.storage or node.funcspec or node.bitsize is not None
                or node.name in address_taken):
            return False
        if 'volatile' in (node.quals or []):
            return False
        kind = node.type
        if isinstance(kind, ca.PtrDecl):
            # A pointer object has fixed size. Exclude explicit variably modified
            # and function declarators, and tags that could change another
            # declaration's binding when their order changes.
            return not has(kind, (ca.ArrayDecl, ca.FuncDecl)) and not defines_tag(kind)
        return (isinstance(kind, ca.TypeDecl)
                and isinstance(kind.type, ca.IdentifierType)
                and set(kind.type.names) <= {
                    'signed', 'unsigned', 'char', 'short', 'int', 'long',
                    'float', 'double', '_Bool'})

    def repeatable(node):
        # No calls, assignments, increments, compound literals or VLA type
        # expressions. The loop rewrite keeps the number/order of evaluations.
        if isinstance(node, (ca.ID, ca.Constant)):
            return True
        if isinstance(node, ca.BinaryOp):
            return repeatable(node.left) and repeatable(node.right)
        if isinstance(node, ca.UnaryOp) and node.op in ('!', '+', '-', '~', '*', '&'):
            return repeatable(node.expr)
        if isinstance(node, ca.StructRef):
            return repeatable(node.name) and isinstance(node.field, ca.ID)
        if isinstance(node, ca.ArrayRef):
            return repeatable(node.name) and repeatable(node.subscript)
        return False

    ret_type = fn.decl.type.type
    returns_void = (isinstance(ret_type, ca.TypeDecl)
                    and isinstance(ret_type.type, ca.IdentifierType)
                    and ret_type.type.names == ['void'])
    jumps = (ca.Goto, ca.Label, ca.Case, ca.Default, ca.Break, ca.Continue)

    class Visitor(ca.NodeVisitor):
        def visit_If(self, node):
            if ('reverse_cond' in enabled and inside(node) and node.iffalse is not None
                    and node.cond is not None):
                def reverse(node=node):
                    node.cond = (node.cond.expr if isinstance(node.cond, ca.UnaryOp)
                                 and node.cond.op == '!' else ca.UnaryOp('!', node.cond))
                    node.iftrue, node.iffalse = node.iffalse, node.iftrue
                result.append(Plan('reverse_cond', reverse))
            self.generic_visit(node)

        def visit_TernaryOp(self, node):
            if ('reverse_cond' in enabled and inside(node) and node.cond is not None
                    and node.iftrue is not None and node.iffalse is not None):
                def reverse(node=node):
                    node.cond = (node.cond.expr if isinstance(node.cond, ca.UnaryOp)
                                 and node.cond.op == '!' else ca.UnaryOp('!', node.cond))
                    node.iftrue, node.iffalse = node.iffalse, node.iftrue
                result.append(Plan('reverse_cond', reverse))
            self.generic_visit(node)

        def visit_Compound(self, node):
            items = node.block_items or []
            if 'reorder_uninitialized_decls' in enabled:
                for i, (left, right) in enumerate(zip(items, items[1:])):
                    if (plain_decl(left) and plain_decl(right) and left.name != right.name
                            and inside(left) and inside(right)):
                        def swap(node=node, i=i):
                            node.block_items[i], node.block_items[i+1] = (
                                node.block_items[i+1], node.block_items[i])
                        result.append(Plan('reorder_uninitialized_decls', swap))
            if 'guard_return' in enabled:
                for i, item in enumerate(items):
                    if (not isinstance(item, ca.If) or item.iffalse is not None
                            or not inside(item) or has(item, jumps)
                            # Names introduced by the controlling expression
                            # belong to the selection statement's own scope.
                            or defines_tag(item.cond)
                            or has(item.cond, (ca.Decl, ca.Typedef))):
                        continue
                    suffix = items[i+1:]
                    if (terminal_return(item.iftrue) and suffix
                            and terminal_return(suffix[-1])
                            and all(inside(n) and not has(n, jumps) for n in suffix)):
                        def guard(node=node, item=item, suffix=suffix, i=i):
                            node.block_items[i:] = [
                                ca.If(ca.UnaryOp('!', item.cond), ca.Compound(suffix), None),
                                item.iftrue]
                        result.append(Plan('guard_return', guard))
                    elif (node is fn.body and returns_void and not suffix
                          and not terminal_return(item.iftrue)):
                        def guard_void(node=node, item=item, i=i):
                            node.block_items[i:] = [
                                ca.If(ca.UnaryOp('!', item.cond), ca.Return(None), None),
                                item.iftrue]
                        result.append(Plan('guard_return', guard_void))
            self.generic_visit(node)

        def visit_While(self, node):
            # Replacing a statement requires its parent. Compound statement
            # positions cover the natural loop forms this adapter supports.
            self.generic_visit(node)

    Visitor().visit(fn.body)

    if 'while_to_guarded_do' in enabled:
        class LoopVisitor(ca.NodeVisitor):
            def visit_Compound(self, parent):
                for i, node in enumerate(parent.block_items or []):
                    if (isinstance(node, ca.While) and inside(node)
                            and node.cond is not None and repeatable(node.cond)
                            and not has(node.stmt, (ca.Goto, ca.Label, ca.Case, ca.Default))):
                        def guarded_do(parent=parent, node=node, i=i):
                            parent.block_items[i] = ca.If(
                                copy.deepcopy(node.cond),
                                ca.DoWhile(node.cond, node.stmt), None)
                        result.append(Plan('while_to_guarded_do', guarded_do))
                self.generic_visit(parent)
        LoopVisitor().visit(fn.body)
    return result
