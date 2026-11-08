import os
import numpy as np
import matplotlib.pyplot as plt


MODELOS = {
    "O(1)": lambda n: np.ones_like(n),
    "O(log n)": lambda n: np.log(n),
    "O(n)": lambda n: n,
    "O(n log n)": lambda n: n * np.log(n),
    "O(n^1.5)": lambda n: n ** 1.5,
    "O(n^2)": lambda n: n ** 2,
    "O(n^3)": lambda n: n ** 3,
}


def carregar_dados(caminho):
    if not os.path.exists(caminho):
        raise FileNotFoundError(f"Arquivo {caminho} não encontrado.")

    dados = np.loadtxt(caminho, delimiter=",", comments="#")
    return dados[:, 0].astype(float), dados[:, 1].astype(float)


def ajustar_lei_potencia(n, y):
    """Ajuste log-log para estimar y = c * n^k."""
    coef = np.polyfit(np.log(n), np.log(y), deg=1)
    return coef[0], np.exp(coef[1])


def ajustar_modelo(n, y, f_n):
    c = np.sum(y * f_n) / np.sum(f_n ** 2)
    previsto = c * f_n
    ss_res = np.sum((y - previsto) ** 2)
    ss_tot = np.sum((y - np.mean(y)) ** 2)
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot > 0 else 0.0
    return c, r2


def analisar_complexidade(n, y):
    resultados = {}

    for nome, funcao in MODELOS.items():
        c, r2 = ajustar_modelo(n, y, funcao(n))
        resultados[nome] = {"c": c, "r2": r2}

    melhor = max(resultados, key=lambda k: resultados[k]["r2"])
    return resultados, melhor


def plotar_resultados(n, y, resultados, melhor, titulo):
    n_linha = np.linspace(min(n), max(n), 500)
    c = resultados[melhor]["c"]

    plt.figure(figsize=(10, 6))
    plt.scatter(n, y, color="red", label="Medições (mediana)", zorder=5)
    plt.plot(
        n_linha,
        c * MODELOS[melhor](n_linha),
        color="blue",
        linestyle="--",
        label=f"Ajuste: {melhor} (R²={resultados[melhor]['r2']:.4f})",
    )
    plt.title(f"Análise de Desempenho: {titulo}")
    plt.xlabel("Tamanho da entrada (n)")
    plt.ylabel("Ciclos de CPU por operação")
    plt.legend()
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.show()


if __name__ == "__main__":
    caminhos = [
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

    for caminho in caminhos:
        try:
            n, y = carregar_dados(caminho)
            k, _ = ajustar_lei_potencia(n, y)
            resultados, melhor = analisar_complexidade(n, y)

            print(caminho)
            print(f"  expoente k (log-log): {k:.4f}")
            for classe, res in resultados.items():
                print(f"  {classe:10} -> R² = {res['r2']:.6f}")
            print(f"  => {melhor}\n")

            plotar_resultados(n, y, resultados, melhor, caminho)
        except Exception as e:
            print(f"Erro em {caminho}: {e}")
