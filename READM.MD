notas

inserir(timestamp)
remover(timestamp)
listar_ordenado()

- inserir(timestamp) / insert(timestamp): Adiciona um novo registro à estrutura, preservando
  suas propriedades estruturais e invariantes.

- remover(timestamp) / remove(timestamp): Localiza e elimina o registro especificado,
  executando as reestruturações ou balanceamentos necessários.

- listar_ordenado() / sorted(): Recupera e descarrega todos os registros da estrutura
  em ordem cronológica ascendente estrita, segundo uma das duas
  estratégias definidas pré-ordenação na listagem ou inserção ordenada — a
  escolha e a ponderação entre elas fazem parte da análise do grupo.

70% inserções, 15% remoções e 15% listagens ordenadas