import SwiftUI
import WebKit

@main
struct NoiseApp: App {
    var body: some Scene { WindowGroup { ConnectionView() } }
}

struct ConnectionView: View {
    @AppStorage("consoleAddress") private var address = ""
    @State private var destination: URL?
    @State private var error = ""
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
            .toolbar {
                if destination != nil {
                    ToolbarItem(placement: .topBarLeading) {
                        Button("Desligar") { destination = nil }
                    }
                }
            }
        }.tint(.mint).preferredColorScheme(.dark)
    }
    private func connect() {
        var value = address.trimmingCharacters(in: .whitespacesAndNewlines)
        if !value.contains("://") { value = "http://" + value }
        guard var parts = URLComponents(string: value), parts.scheme == "http",
              parts.user == nil, parts.password == nil, let host = parts.host,
              isPrivateIPv4(host), let port = parts.port, (1...65535).contains(port),
              parts.query == nil, parts.path == "" || parts.path == "/" else {
            error = "Introduz o endereço IP local e a porta apresentados na PS4."
            return
        }
        parts.path = "/"
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
