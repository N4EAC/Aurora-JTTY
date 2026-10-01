import AppKit

// Original vector artwork, rendered at every macOS icon size.
func drawIcon(_ size: Int) -> NSBitmapImageRep {
    let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: size, pixelsHigh: size, bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: bitmap)
    let scale = CGFloat(size) / 1024
    let transform = NSAffineTransform(); transform.scale(by: scale); transform.concat()
    let tile = NSBezierPath(roundedRect: NSRect(x: 32, y: 32, width: 960, height: 960), xRadius: 210, yRadius: 210)
    NSGradient(starting: NSColor(calibratedRed: 0.06, green: 0.16, blue: 0.28, alpha: 1), ending: NSColor(calibratedRed: 0.02, green: 0.07, blue: 0.14, alpha: 1))!.draw(in: tile, angle: -75)
    let bubble = NSBezierPath(roundedRect: NSRect(x: 174, y: 292, width: 676, height: 482), xRadius: 108, yRadius: 108)
    NSColor(calibratedRed: 0.12, green: 0.82, blue: 0.75, alpha: 1).setFill(); bubble.fill()
    let tail = NSBezierPath(); tail.move(to: NSPoint(x: 278, y: 330)); tail.line(to: NSPoint(x: 265, y: 182)); tail.line(to: NSPoint(x: 442, y: 320)); tail.close(); tail.fill()
    let wave = NSBezierPath(); wave.move(to: NSPoint(x: 250, y: 532))
    for point in [NSPoint(x: 320,y: 532),NSPoint(x: 365,y: 628),NSPoint(x: 424,y: 423),NSPoint(x: 489,y: 683),NSPoint(x: 554,y: 386),NSPoint(x: 613,y: 601),NSPoint(x: 665,y: 532),NSPoint(x: 774,y: 532)] { wave.line(to: point) }
    wave.lineWidth = 38; wave.lineCapStyle = .round; wave.lineJoinStyle = .round
    NSColor(calibratedRed: 0.03, green: 0.14, blue: 0.23, alpha: 1).setStroke(); wave.stroke()
    NSGraphicsContext.restoreGraphicsState()
    return bitmap
}
let root = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
let iconset = root.appendingPathComponent("JTTY.iconset")
try FileManager.default.createDirectory(at: iconset, withIntermediateDirectories: true)
for base in [16, 32, 128, 256, 512] {
    for factor in [1, 2] {
        let name = "icon_\(base)x\(base)\(factor == 2 ? "@2x" : "").png"
        try drawIcon(base * factor).representation(using: .png, properties: [:])!.write(to: iconset.appendingPathComponent(name))
    }
}
try drawIcon(1024).representation(using: .png, properties: [:])!.write(to: root.appendingPathComponent("JTTY.png"))
