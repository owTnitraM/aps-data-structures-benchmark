import os
import numpy as np
import matplotlib.pyplot as plt


def carregar_dados(caminho_arquivo):
    """
    Lê o arquivo gerado pelo programa em C/C++.
    O arquivo deve ter o formato: n,mediana_ciclos
    """

    if not os.path.exists(caminho_arquivo):
        raise FileNotFoundError(f"Arquivo {caminho_arquivo} não encontrado.")

    # Carrega os dados ignorando linhas de comentário (#)
    dados = np.loadtxt(caminho_arquivo, delimiter=",", comments="#")
    n = dados[:, 0].astype(float)
    y = dados[:, 1].astype(float)

    return n, y


def ajustar_lei_potencia(n, y):
    """
    Método 1: Ajuste log-log para estimar o expoente k (y = c * n^k).
    Útil para identificar a ordem de grandeza inicial.
    """

    log_n = np.log(n)
    log_y = np.log(y)
    coef = np.polyfit(log_n, log_y, deg=1)
    k = coef[0]
    c = np.exp(coef[1])

    return k, c


def ajustar_modelo(n, y, f_n):
    """
    Método 2: Mínimos quadrados em escala log (resíduo relativo).
    Ajusta log y = log c + log f(n) e calcula o R2 no espaço log.
    """

    log_y = np.log(y)
    log_f = np.log(f_n)

    log_c = np.mean(log_y - log_f)
    c = np.exp(log_c)
    log_previsto = log_f + log_c

    ss_res = np.sum((log_y - log_previsto) ** 2)
    ss_tot = np.sum((log_y - np.mean(log_y)) ** 2)
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot > 0 else 0.0

    return c, r2, c * f_n


def analisar_complexidade(n, y):
    """
    Testa as complexidades candidatas e retorna os resultados.
    """
    modelos = {
        "O(1)": np.ones_like(n),
        "O(log n)": np.log(n),
        "O(n)": n,
        "O(n log n)": n * np.log(n),
        "O(n^1.5)": n**1.5,
        "O(n^2)": n**2,
        "O(n^3)": n**3,
    }
    resultados = {}
    for nome, f_n in modelos.items():
        c, r2, y_previsto = ajustar_modelo(n, y, f_n)
        resultados[nome] = {"c": c, "r2": r2, "previsto": y_previsto}

    # Define o melhor modelo baseado no maior R2
    melhor_nome = max(resultados, key=lambda k: resultados[k]["r2"])
    return resultados, melhor_nome


def plotar_resultados(
    n, y, resultados, melhor_nome, titulo_grafico, complex_esperada: str
):
    """
    Gera o gráfico comparativo.
    """

    plt.figure(figsize=(10, 6))

    # Pontos experimentais
    plt.scatter(n, y, color="red", label="Medições (Mediana)", zorder=5)

    # Curva de ajuste do melhor modelo
    # Criamos um n_linha suave para a curva não ficar "quebrada"
    n_linha = np.linspace(min(n), max(n), 500)

    # Define a função para o n_linha baseada no melhor modelo
    if melhor_nome == "O(1)":
        f_linha = np.ones_like(n_linha)
    elif melhor_nome == "O(log n)":
        f_linha = np.log(n_linha)
    elif melhor_nome == "O(n)":
        f_linha = n_linha
    elif melhor_nome == "O(n log n)":
        f_linha = n_linha * np.log(n_linha)
    elif melhor_nome == "O(n^1.5)":
        f_linha = n_linha**1.5
    elif melhor_nome == "O(n^2)":
        f_linha = n_linha**2
    elif melhor_nome == "O(n^3)":
        f_linha = n_linha**3
    c_otimo = resultados[melhor_nome]["c"]
    plt.plot(
        n_linha,
        c_otimo * f_linha,
        color="blue",
        linestyle="-",
        label=f'Ajuste: {melhor_nome} (R² log={resultados[melhor_nome]["r2"]:.4f})',
    )
    plt.plot(
        n_linha,
        c_otimo * f_linha,
        color="blue",
        linestyle="-",
        label=f"Esperado: {complex_esperada}",
    )

    plt.title(f"Análise de Desempenho: {titulo_grafico}")
    plt.xlabel("Tamanho da Entrada (n)")
    plt.ylabel("Ciclos de CPU Estimados")
    plt.legend()
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.show()


# --- EXECUÇÃO PRINCIPAL ---
if __name__ == "__main__":

    # Nome do arquivo gerado pelo C/C++
    CAMINHOS = [
        "analysis/avl_insert.txt",
        "analysis/avl_list.txt",
        "analysis/avl_remove.txt",
        "analysis/sll_insert.txt",
        "analysis/sll_list.txt",
        "analysis/sll_remove.txt",
        "analysis/vector_insert.txt",
        "analysis/vector_list.txt",
        "analysis/vector_remove.txt",
    ]

    complexidades = [
        "O(log n)",
        "O(n)",
        "O(log n)",
        "O(1)",
        "O(n log n)",
        "O(n)",
        "O(n)",
        "O(n)",
        "O(n)",
    ]

    # NOME_ARQUIVO = "analysis/sll_insert.txt"
    for i, path in enumerate(CAMINHOS):
        NOME_ARQUIVO = path
        try:
            n, y = carregar_dados(NOME_ARQUIVO)

            # --- ALTERNATIVA: CARREGAMENTO MANUAL DE DADOS PARA TESTE ---
            # Comente a linha: n, y = carregar_dados(NOME_ARQUIVO)
            # E utilize um dos blocos abaixo para testar o fitting:
            # Exemplo 1: Simulação de comportamento Linear O(n)
            # n = np.array([1000, 5000, 10000, 50000, 100000])
            # y = np.array([1200, 5100, 10500, 50200, 100800])
            # Exemplo 2: Simulação de comportamento Logarítmico O(log n)
            # n = np.array([1000, 5000, 10000, 50000, 100000])
            # y = np.array([10, 12, 13, 15, 17])
            # Exemplo 3: Simulação de comportamento Quadrático O(n^2)
            # n = np.array([1000, 5000, 10000, 50000, 100000])
            # y = np.array([1000000, 25000000, 100000000, 2500000000, 10000000000])
            # Exemplo 4: Simulação de O(n log n) - Comum em ordenação
            # n = np.array([1000, 5000, 10000, 50000, 100000])
            # y = n * np.log(n) * np.random.uniform(0.99,1.2,5) # (Simulado com um ruído de 20%)
            # -----------------------------------------------------------

            # Passo 1: Lei de Potência
            k_exp, c_pot = ajustar_lei_potencia(n, y)
            # print(f"Expoente k estimado (log-log): {k_exp:.4f}")

            # Passo 2: Teste de classes específicas
            resultados, melhor = analisar_complexidade(n, y)
            """ print("\nResultados por classe de complexidade:")
            for classe, res in resultados.items():
                print(f" {classe:10} -> R² log = {res['r2']:.6f}") """

            nome_estrutura: str = path[9:]
            nome_estrutura = nome_estrutura[:-4]
            print(
                f"\nConclusão: {nome_estrutura} comporta-se como {melhor} | Esperado {complexidades[i]} | {"Correto" if melhor == complexidades[i] else "Errado!"}"
            )

            # Passo 3: Visualização
            plotar_resultados(
                n, y, resultados, melhor, nome_estrutura, complexidades[i]
            )
        except Exception as e:
            print(f"Erro: {e}")
