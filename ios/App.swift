import SwiftUI
import WebKit
import VisionKit
import AVFoundation

@main
struct NoiseApp: App {
    var body: some Scene { WindowGroup { ConnectionView() } }
}

struct ConnectionView: View {
    @AppStorage("consoleAddress") private var address = ""
    @State private var destination: URL?
    @State private var error = ""
    @State private var code = ""
    @State private var scanning = false
    var body: some View {
        NavigationStack {
            Group {
                if let url = destination {
                    ConsolePage(url: url)
                } else {
                    Form {
                        Section {
                            HStack(spacing: 16) {
                                Image(uiImage: UIImage(named: "Brand.png") ?? UIImage())
                                    .resizable().scaledToFit().frame(width: 80, height: 80)
                                    .clipShape(RoundedRectangle(cornerRadius: 18))
                                Text("h1pNoise").font(.largeTitle.bold())
                            }
                            Text("Controla as transferências da tua PS4 pelo iPhone.")
                        }
                        Section("Endereço apresentado na consola") {
                            TextField("http://192.168.1.100:8787", text: $address)
                                .keyboardType(.URL).textInputAutocapitalization(.never)
                                .autocorrectionDisabled()
                            TextField("Código apresentado na consola", text: $code)
                                .textInputAutocapitalization(.never).autocorrectionDisabled()
                            Button("Ler QR da consola", systemImage: "qrcode.viewfinder", action: scan)
                            Button("Ligar à consola", action: connect)
                            if !error.isEmpty { Text(error).foregroundStyle(.red) }
                        }
                        Section {
                            Text("Abre a h1pNoise na PS4 e liga o iPhone à mesma rede Wi-Fi. Permite o acesso à rede local quando o iOS pedir.")
                            Text("Podes colar o endereço completo do QR. O código de ligação só é usado durante esta sessão.")
                        }
                    }
                }
            }
            .navigationTitle(destination == nil ? "A tua PS4" : "h1pNoise")
            .navigationBarTitleDisplayMode(.inline)
            .sheet(isPresented: $scanning) {
                NavigationStack {
                    QRScanner { value in
                        scanning = false
                        address = value
                        code = ""
                        connect()
                    } onError: { message in
                        scanning = false
                        error = message
                    }
                    .ignoresSafeArea(edges: .bottom)
                    .navigationTitle("Aponta ao QR da PS4")
                    .navigationBarTitleDisplayMode(.inline)
                    .toolbar { Button("Cancelar") { scanning = false } }
                }
            }
            .toolbar {
                if destination != nil {
                    ToolbarItem(placement: .topBarLeading) {
                        Button("Desligar") { destination = nil }
                    }
                }
            }
        }.tint(.mint).preferredColorScheme(.dark)
    }
    private func scan() {
        guard DataScannerViewController.isSupported else {
            error = "Este iPhone não suporta esta leitura de QR. Introduz o IP e o código manualmente."
            return
        }
        AVCaptureDevice.requestAccess(for: .video) { allowed in
            DispatchQueue.main.async {
                if allowed && DataScannerViewController.isAvailable { scanning = true }
                else { error = "Permite o acesso à Câmara nas Definições do iPhone ou introduz o IP e o código manualmente." }
            }
        }
    }
    private func connect() {
        var value = address.trimmingCharacters(in: .whitespacesAndNewlines)
        if !value.contains("://") { value = "http://" + value }
        guard var parts = URLComponents(string: value), parts.scheme == "http",
              parts.user == nil, parts.password == nil, let host = parts.host,
              isPrivateIPv4(host), (1...65535).contains(parts.port ?? 8787),
              parts.query == nil, parts.path == "" || parts.path == "/" else {
            error = "Introduz o endereço IP local e a porta apresentados na PS4."
            return
        }
        let fragment = URLComponents(string: "http://local/?" + (parts.fragment ?? ""))
        let qrCode = fragment?.queryItems?.first(where: { $0.name == "code" })?.value
        let pairingCode = (qrCode ?? code).trimmingCharacters(in: .whitespacesAndNewlines)
        guard !pairingCode.isEmpty else {
            error = "Introduz o código apresentado na PS4 ou lê o QR da consola."
            return
        }
        parts.port = parts.port ?? 8787
        parts.path = "/"
        var pairing = URLComponents()
        pairing.queryItems = [URLQueryItem(name: "code", value: pairingCode)]
        parts.percentEncodedFragment = pairing.percentEncodedQuery
        destination = parts.url
        parts.fragment = nil
        address = parts.string ?? ""
        error = ""
    }
    private func isPrivateIPv4(_ host: String) -> Bool {
        let fields = host.split(separator: ".", omittingEmptySubsequences: false)
        let values = fields.compactMap { Int($0) }
        guard fields.count == 4, values.count == 4,
              values.allSatisfy({ (0...255).contains($0) }) else { return false }
        return values[0] == 10 || (values[0] == 192 && values[1] == 168)
            || (values[0] == 172 && (16...31).contains(values[1]))
    }
}

struct QRScanner: UIViewControllerRepresentable {
    let onScan: (String) -> Void
    let onError: (String) -> Void
    func makeCoordinator() -> Coordinator { Coordinator(parent: self) }
    func makeUIViewController(context: Context) -> DataScannerViewController {
        let scanner = DataScannerViewController(recognizedDataTypes: [.barcode(symbologies: [.qr])],
            qualityLevel: .balanced, recognizesMultipleItems: false,
            isGuidanceEnabled: true, isHighlightingEnabled: true)
        scanner.delegate = context.coordinator
        do { try scanner.startScanning() }
        catch { DispatchQueue.main.async { onError("Não foi possível abrir a câmara. Introduz o IP e o código manualmente.") } }
        return scanner
    }
    func updateUIViewController(_ controller: DataScannerViewController, context: Context) {}
    static func dismantleUIViewController(_ controller: DataScannerViewController, coordinator: Coordinator) {
        controller.stopScanning()
    }
    class Coordinator: NSObject, DataScannerViewControllerDelegate {
        let parent: QRScanner
        var delivered = false
        init(parent: QRScanner) { self.parent = parent }
        func dataScanner(_ scanner: DataScannerViewController, didAdd addedItems: [RecognizedItem], allItems: [RecognizedItem]) {
            for item in addedItems {
                if case .barcode(let barcode) = item, let value = barcode.payloadStringValue, !delivered {
                    delivered = true
                    scanner.stopScanning()
                    parent.onScan(value)
                    return
                }
            }
        }
        func dataScanner(_ scanner: DataScannerViewController, becameUnavailableWithError error: DataScannerViewController.ScanningUnavailable) {
            parent.onError("A câmara ficou indisponível. Tenta novamente ou introduz o IP e o código.")
        }
    }
}

struct ConsolePage: UIViewRepresentable {
    let url: URL
    func makeCoordinator() -> Coordinator { Coordinator(url: url) }
    func makeUIView(context: Context) -> WKWebView {
        let config = WKWebViewConfiguration()
        config.websiteDataStore = .nonPersistent()
        let view = WKWebView(frame: .zero, configuration: config)
        view.navigationDelegate = context.coordinator
        view.isOpaque = false
        view.load(URLRequest(url: url, timeoutInterval: 20))
        return view
    }
    func updateUIView(_ view: WKWebView, context: Context) {}
    class Coordinator: NSObject, WKNavigationDelegate {
        let url: URL
        init(url: URL) { self.url = url }
        func webView(_ webView: WKWebView, decidePolicyFor action: WKNavigationAction,
                     decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
            guard let target = action.request.url else { decisionHandler(.cancel); return }
            decisionHandler(target.scheme == url.scheme && target.host == url.host
                && target.port == url.port ? .allow : .cancel)
        }
        func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
            let message = "Não foi possível ligar. Confirma o IP, a rede Wi-Fi e a permissão de Rede local nas Definições do iPhone. Toca em Desligar para tentar novamente."
            webView.loadHTMLString("<meta name='viewport' content='width=device-width'><body style='background:#111827;color:white;font:20px -apple-system;padding:24px'>" + message + "</body>", baseURL: url)
        }
    }
}
