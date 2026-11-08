// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Package the existing project icon as Apple's opaque, full-square iOS source icon.
import AppKit
import CoreGraphics
import Foundation

let source = URL(fileURLWithPath: CommandLine.arguments[1])
let output = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
let files = FileManager.default
let iconSet = output.appendingPathComponent("AppIcon.appiconset", isDirectory: true)
try files.createDirectory(at: iconSet, withIntermediateDirectories: true)
guard let image = NSImage(contentsOf: source),
      let original = image.cgImage(forProposedRect: nil, context: nil, hints: nil),
      let space = CGColorSpace(name: CGColorSpace.sRGB),
      let context = CGContext(data: nil, width: 1024, height: 1024, bitsPerComponent: 8,
          bytesPerRow: 0, space: space, bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue) else {
    fatalError("Could not read the OpenRAC icon")
}
// Quartz needs 32-bit RGB storage; the former 24-bit AppKit context was nil and drew nothing.
// Ignore the fourth component so the App Store image remains opaque.
let rect = CGRect(x: 0, y: 0, width: 1024, height: 1024)
context.setFillColor(CGColor(srgbRed: 0.95, green: 0.70, blue: 0.15, alpha: 1))
context.fill(rect)
context.interpolationQuality = .high
context.draw(original, in: rect)
guard let rendered = context.makeImage(),
      let png = NSBitmapImageRep(cgImage: rendered).representation(using: .png, properties: [:]) else {
    fatalError("Could not encode the OpenRAC icon")
}
try png.write(to: iconSet.appendingPathComponent("AppIcon.png"))
let manifest = """
{"images":[{"filename":"AppIcon.png","idiom":"universal","platform":"ios","size":"1024x1024"}],"info":{"author":"xcode","version":1}}
"""
try manifest.write(to: iconSet.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8)
