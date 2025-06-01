#!/bin/bash

# Script para testar múltiplos arquivos .in do TSP (versões linear e paralela)

# Cores para output colorido
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}Compilando TSP Solver (versão linear)...${NC}"
g++ -O3 -std=c++17 src/main_tsp.cpp -o main_tsp_linear

if [ $? -ne 0 ]; then
    echo -e "${RED}Erro na compilação da versão linear!${NC}"
    exit 1
fi

echo -e "${BLUE}Compilando TSP Solver (versão paralela)...${NC}"
g++ -fopenmp -O3 -std=c++17 src/main_tsp_p.cpp -o main_tsp_p

if [ $? -ne 0 ]; then
    echo -e "${RED}Erro na compilação da versão paralela!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Ambas compilações bem-sucedidas!${NC}"
echo "=================================="

# Configurar número de threads para OpenMP
export OMP_NUM_THREADS=8

# Encontrar todos os arquivos .in
input_files=(*.in)

if [ ${#input_files[@]} -eq 0 ] || [ ! -f "${input_files[0]}" ]; then
    echo -e "${RED}Nenhum arquivo .in encontrado no diretório atual!${NC}"
    exit 1
fi

echo -e "${CYAN}Arquivos encontrados: ${#input_files[@]}${NC}"
echo -e "${CYAN}Usando ${OMP_NUM_THREADS} threads para versão paralela${NC}"
echo "=================================="

# Criar arquivo de resultados
results_file="src/outputs/comparison_results_$(date +%Y%m%d_%H%M%S).txt"
csv_file="src/outputs/comparison_results_$(date +%Y%m%d_%H%M%S).csv"

echo "TSP Solver Comparison Results - $(date)" > "$results_file"
echo "=======================================" >> "$results_file"

# Criar cabeçalho do CSV
echo "Arquivo,Cidades,Iteracoes,Restarts,Seed,Tempo_Linear,Tour_Linear,Tempo_Paralelo,Tour_Paralelo,Speedup,Melhoria_Qualidade" > "$csv_file"

# Função para extrair informações do arquivo .in
extract_info() {
    local file=$1
    local first_line=$(head -n 1 "$file")
    local iterations=$(echo $first_line | cut -d' ' -f1)
    local restarts=$(echo $first_line | cut -d' ' -f2)
    local seed=$(echo $first_line | cut -d' ' -f3)
    local cities=$(tail -n +2 "$file" | wc -l)
    echo "$iterations $restarts $seed $cities"
}

# Função para extrair tour length do output
extract_tour_length() {
    local output="$1"
    echo "$output" | grep "Tour length:" | cut -d':' -f2 | tr -d ' '
}

total_linear_time=0
total_parallel_time=0
better_linear=0
better_parallel=0
equal_results=0

# Testar cada arquivo
for input_file in "${input_files[@]}"; do
    echo -e "${YELLOW}Processando: $input_file${NC}"
    
    # Extrair informações do arquivo
    info=$(extract_info "$input_file")
    iterations=$(echo $info | cut -d' ' -f1)
    restarts=$(echo $info | cut -d' ' -f2)  
    seed=$(echo $info | cut -d' ' -f3)
    cities=$(echo $info | cut -d' ' -f4)
    
    echo "" >> "$results_file"
    echo "========================================" >> "$results_file"
    echo "Arquivo: $input_file" >> "$results_file"
    echo "Cidades: $cities | Iterações: $iterations | Restarts: $restarts | Seed: $seed" >> "$results_file"
    echo "========================================" >> "$results_file"
    
    echo -e "  ${CYAN}Configuração: $cities cidades, $iterations iterações, $restarts restarts${NC}"
    
    # TESTE VERSÃO LINEAR
    echo -e "  ${BLUE}Executando versão LINEAR...${NC}"
    start_time=$(date +%s.%N)
    linear_output=$(./main_tsp_linear < "$input_file" 2>&1)
    end_time=$(date +%s.%N)
    linear_time=$(echo "$end_time - $start_time" | bc -l)
    linear_tour_length=$(extract_tour_length "$linear_output")
    
    echo "VERSÃO LINEAR:" >> "$results_file"
    echo "$linear_output" >> "$results_file"
    printf "Tempo Linear: %.3f segundos\n" "$linear_time" >> "$results_file"
    echo "" >> "$results_file"
    
    printf "    ✓ Linear: %.3fs, Tour: %s\n" "$linear_time" "$linear_tour_length"
    
    # TESTE VERSÃO PARALELA  
    echo -e "  ${PURPLE}Executando versão PARALELA...${NC}"
    start_time=$(date +%s.%N)
    parallel_output=$(./main_tsp_p < "$input_file" 2>&1)
    end_time=$(date +%s.%N)
    parallel_time=$(echo "$end_time - $start_time" | bc -l)
    parallel_tour_length=$(extract_tour_length "$parallel_output")
    
    echo "VERSÃO PARALELA:" >> "$results_file"
    echo "$parallel_output" >> "$results_file"
    printf "Tempo Paralelo: %.3f segundos\n" "$parallel_time" >> "$results_file"
    echo "" >> "$results_file"
    
    printf "    ✓ Paralelo: %.3fs, Tour: %s\n" "$parallel_time" "$parallel_tour_length"
    
    # CALCULAR SPEEDUP E COMPARAÇÕES
    if [[ "$linear_time" != "0" && "$parallel_time" != "0" ]]; then
        speedup=$(echo "scale=2; $linear_time / $parallel_time" | bc -l)
        speedup_percent=$(echo "scale=1; ($linear_time - $parallel_time) * 100 / $linear_time" | bc -l)
    else
        speedup="N/A"
        speedup_percent="N/A"
    fi
    
    # Comparar qualidade das soluções
    quality_comparison="="
    if [[ "$linear_tour_length" != "" && "$parallel_tour_length" != "" ]]; then
        quality_diff=$(echo "$linear_tour_length - $parallel_tour_length" | bc -l)
        quality_diff_abs=$(echo "$quality_diff" | sed 's/-//')
        
        if (( $(echo "$quality_diff > 0.001" | bc -l) )); then
            quality_comparison="Paralelo melhor"
            ((better_parallel++))
        elif (( $(echo "$quality_diff < -0.001" | bc -l) )); then
            quality_comparison="Linear melhor"  
            ((better_linear++))
        else
            quality_comparison="Iguais"
            ((equal_results++))
        fi
    fi
    
    # EXIBIR COMPARAÇÃO COLORIDA
    echo -e "  ${GREEN}COMPARAÇÃO:${NC}"
    if [[ "$speedup" != "N/A" ]]; then
        if (( $(echo "$speedup > 1.1" | bc -l) )); then
            echo -e "    ${GREEN}⚡ Speedup: ${speedup}x (${speedup_percent}% mais rápido)${NC}"
        elif (( $(echo "$speedup < 0.9" | bc -l) )); then
            echo -e "    ${RED}🐌 Slowdown: ${speedup}x (versão paralela mais lenta)${NC}"
        else
            echo -e "    ${YELLOW}📊 Speedup: ${speedup}x (diferença pequena)${NC}"
        fi
    fi
    
    if [[ "$quality_comparison" == "Paralelo melhor" ]]; then
        echo -e "    ${GREEN}🎯 Qualidade: Paralelo encontrou solução melhor${NC}"
    elif [[ "$quality_comparison" == "Linear melhor" ]]; then
        echo -e "    ${RED}🎯 Qualidade: Linear encontrou solução melhor${NC}"
    else
        echo -e "    ${CYAN}🎯 Qualidade: Resultados equivalentes${NC}"
    fi
    
    # Salvar no CSV
    echo "$input_file,$cities,$iterations,$restarts,$seed,$linear_time,$linear_tour_length,$parallel_time,$parallel_tour_length,$speedup,$quality_comparison" >> "$csv_file"
    
    # Adicionar aos totais
    total_linear_time=$(echo "$total_linear_time + $linear_time" | bc -l)
    total_parallel_time=$(echo "$total_parallel_time + $parallel_time" | bc -l)
    
    echo "COMPARAÇÃO:" >> "$results_file"
    echo "Speedup: $speedup" >> "$results_file"
    echo "Qualidade: $quality_comparison" >> "$results_file"
    echo "" >> "$results_file"
done

echo "=================================="
echo -e "${GREEN}Todos os testes concluídos!${NC}"
echo -e "${CYAN}Resultados detalhados salvos em: $results_file${NC}"
echo -e "${CYAN}Resultados CSV salvos em: $csv_file${NC}"

# RELATÓRIO FINAL COLORIDO
echo ""
echo -e "${YELLOW}📊 RELATÓRIO FINAL DE PERFORMANCE${NC}"
echo -e "${YELLOW}=================================${NC}"

total_files=${#input_files[@]}
overall_speedup=$(echo "scale=2; $total_linear_time / $total_parallel_time" | bc -l)

echo -e "${CYAN}Arquivos processados: $total_files${NC}"
printf "${CYAN}Tempo total linear: %.3fs${NC}\n" "$total_linear_time"
printf "${CYAN}Tempo total paralelo: %.3fs${NC}\n" "$total_parallel_time"

if (( $(echo "$overall_speedup > 1.5" | bc -l) )); then
    echo -e "${GREEN}🚀 Speedup geral: ${overall_speedup}x - EXCELENTE!${NC}"
elif (( $(echo "$overall_speedup > 1.1" | bc -l) )); then
    echo -e "${GREEN}⚡ Speedup geral: ${overall_speedup}x - BOM!${NC}"
elif (( $(echo "$overall_speedup > 0.9" | bc -l) )); then
    echo -e "${YELLOW}📊 Speedup geral: ${overall_speedup}x - NEUTRO${NC}"
else
    echo -e "${RED}🐌 Speedup geral: ${overall_speedup}x - PROBLEMÁTICO${NC}"
fi

echo ""
echo -e "${YELLOW}🎯 QUALIDADE DAS SOLUÇÕES:${NC}"
echo -e "${GREEN}  Paralelo melhor: $better_parallel casos${NC}"
echo -e "${RED}  Linear melhor: $better_linear casos${NC}"  
echo -e "${CYAN}  Resultados iguais: $equal_results casos${NC}"

echo ""
echo -e "${PURPLE}💡 ANÁLISE:${NC}"
if (( better_parallel > better_linear )); then
    echo -e "${GREEN}  ✓ Versão paralela encontra soluções melhores na maioria dos casos${NC}"
elif (( better_linear > better_parallel )); then
    echo -e "${YELLOW}  ⚠ Versão linear encontra soluções melhores - verificar implementação${NC}"
else
    echo -e "${CYAN}  = Ambas versões encontram soluções de qualidade similar${NC}"
fi

if (( $(echo "$overall_speedup > 2.0" | bc -l) )); then
    echo -e "${GREEN}  ✓ Paralelização muito eficiente${NC}"
elif (( $(echo "$overall_speedup > 1.2" | bc -l) )); then
    echo -e "${GREEN}  ✓ Paralelização eficiente${NC}"
else
    echo -e "${YELLOW}  ⚠ Paralelização com potencial de melhoria${NC}"
fi