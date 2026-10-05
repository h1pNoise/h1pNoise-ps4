# h1pNoise 0.1.53 - escrita na raiz USB

Corrige a tentativa de criar o PKG na pen quando a pasta USB abre mas a escrita falha fora da raiz isolada da app. A app tenta novamente com permissões temporárias e, quando o firmware recusa as operações relativas ao diretório, usa o caminho fixo na raiz da pen. Restaura as credenciais antes da rede e da escrita do conteúdo. O download, a verificação e a renomeação usam a pen escolhida, sem copiar o PKG para o disco interno.

O registo /data/pkg/usb-debug.log passa a incluir a versão e os erros das operações de criação, leitura, remoção e renomeação. Se a abertura do ficheiro falhar, a mensagem mostra o código do erro. Ficheiros incompletos são removidos quando possível; ficheiros verificados anteriores são preservados.

## Testar 0.1.52 -> 0.1.53

1. Instala h1pNoise-0.1.52-experimental.pkg manualmente pelo Package Installer do GoldHEN, com a h1pNoise fechada e Enable Background Installation desligado.
2. Abre a 0.1.52, atualiza a página do telemóvel e procura a atualização 0.1.53.
3. Liga a pen exFAT/FAT32 diretamente à PS4. Escolhe Descarregar atualização e Pen USB 1; se a pen estiver no segundo ponto de montagem, escolhe Pen USB 2.
4. O ficheiro fica diretamente na raiz da pen: h1pNoise-update-63.pkg. Não é criada uma subpasta. O destino interno opcional continua /data/pkg/h1pNoise-update-63.pkg.
5. Depois do download verificado, fecha a app e instala o PKG manualmente pelo GoldHEN.

Ambos os PKG desta release contêm a mesma correção USB; a 0.1.52 permite testar imediatamente o download da 0.1.53. A instalação continua manual.

## Validação

177 verificações do atualizador por pacote e testes da interface no PC. Inclui operações USB relativas e por caminho absoluto, permissões restauradas antes da rede, pen ausente, falhas de abertura e fdopen, falta de espaço, interrupção de escrita, remoção da pen, falhas de renomeação e descritores fechados. O acesso PS4 é simulado nestes testes; a escrita na pen numa PS4 real ainda requer confirmação.
