#include <stdio.h>
#include <stdlib.h>

typedef unsigned long long U64;

#define EMPTY  0b00000
#define BORDER 0b00001
#define WHITE  0b00010
#define BLACK  0b00011
#define PAWN   0b00100
#define KNIGHT 0b01000
#define BISHOP 0b01100
#define ROOK   0b10000
#define QUEEN  0b10100
#define KING   0b11000
#define create_figure(color, value) (color | value)
#define get_color(figure) (figure & 0b00011)
#define get_value(figure) (figure & 0b11100)

#define K_castling 0b1000
#define Q_castling 0b0100
#define k_castling 0b0010
#define q_castling 0b0001
#define make_K_castling_is_incorrect() ply->castlings &= 0b0111
#define make_Q_castling_is_incorrect() ply->castlings &= 0b1011
#define make_k_castling_is_incorrect() ply->castlings &= 0b1101
#define make_q_castling_is_incorrect() ply->castlings &= 0b1110
#define make_white_castlings_is_incorrect() ply->castlings &= 0b0011
#define make_black_castlings_is_incorrect() ply->castlings &= 0b1100

#define not_turn_to_move (turn_to_move ^ 1)

#define DRAW 0
#define LOSING -300000

int turn_figures[4] = {QUEEN, ROOK, BISHOP, KNIGHT};

int directions_of_rook[4]   = {1, 10, -1, -10};
int directions_of_bishop[4] = {9, 11, -9, -11};
int directions_of_queen[8]  = {1, 10, -1, -10, 9, 11, -9, -11};
int moves_of_king[8]        = {1, 10, -1, -10, 9, 11, -9, -11};
int moves_of_knight[8]      = {21, 19, 12, 8, -21, -19, -12, -8};

int board64[64] = {
    21,22,23,24,25,26,27,28,
    31,32,33,34,35,36,37,38,
    41,42,43,44,45,46,47,48,
    51,52,53,54,55,56,57,58,
    61,62,63,64,65,66,67,68,
    71,72,73,74,75,76,77,78,
    81,82,83,84,85,86,87,88,
    91,92,93,94,95,96,97,98
};

int board[120];
int place_of_white_king, place_of_black_king;
int turn_to_move;
struct _flags {
    int en_passant;
    int castlings;
    int number_of_insignificant_plies;
    U64 hash;
} begin_ply[1000], *ply;

void print_position()
{
    for(int i = 0; i < 8; i += 1)
    {
        for(int j = 0; j < 8; j += 1)
        {
            switch(board[board64[i*8 + j]])
            {
                case EMPTY: printf(".");
                break; case create_figure(WHITE, KING)  : printf("K");
                break; case create_figure(WHITE, QUEEN) : printf("Q");
                break; case create_figure(WHITE, ROOK)  : printf("R");
                break; case create_figure(WHITE, BISHOP): printf("B");
                break; case create_figure(WHITE, KNIGHT): printf("N");
                break; case create_figure(WHITE, PAWN)  : printf("P");
                break; case create_figure(BLACK, KING)  : printf("k");
                break; case create_figure(BLACK, QUEEN) : printf("q");
                break; case create_figure(BLACK, ROOK)  : printf("r");
                break; case create_figure(BLACK, BISHOP): printf("b");
                break; case create_figure(BLACK, KNIGHT): printf("n");
                break; case create_figure(BLACK, PAWN)  : printf("p");
            }
        }
        printf("\n");
    }
    printf("\n");
}


typedef int Move;
#define create_move(from, to, broken, turn) (from | (to << 7) | (broken << 14) | (turn << 19))
#define move_from(move)   (move & 0b1111111)
#define move_to(move)     ((move >> 7) & 0b1111111)
#define move_broken(move) ((move >> 14) & 0b11111)
#define move_turn(move)   ((move >> 19) & 0b11111)

int history[32][120];
void clear_history()
{
    for(int i = 0; i < 32; i += 1)
    {
        for(int j = 0; j < 120; j += 1)
        {
            history[i][j] = 0;
        }
    }
}

int mvv_lva[32][32];
void mvv_lva_init()
{
    for(int i = 0; i < 32; i += 1)
    {
        int value_of_figure;
        switch(get_value(i))
        {
            case KING  : value_of_figure = 0; break;
            case PAWN  : value_of_figure = 1; break;
            case KNIGHT: value_of_figure = 2; break;
            case BISHOP: value_of_figure = 2; break;
            case ROOK  : value_of_figure = 3; break;
            case QUEEN : value_of_figure = 4; break;
            default: value_of_figure = -1; 
        }
        for(int j = 0; j < 32; j += 1)
        {
            int value_of_broken;
            switch(get_value(j))
            {
                case PAWN  : value_of_broken = 10; break;
                case KNIGHT: value_of_broken = 20; break;
                case BISHOP: value_of_broken = 20; break;
                case ROOK  : value_of_broken = 30; break;
                case QUEEN : value_of_broken = 40; break;
                default: value_of_broken = -1; 
            }
            mvv_lva[i][j] = 100000 * (value_of_broken - value_of_figure);
        }
    }
}

#define hash_table_size 1048575 /*(1 << 20) - 1*/
typedef struct
{
    U64 hash;
    int depth;
    int eval;
    Move bestmove;
    int flag;
} Entry;
Entry hash_table[hash_table_size + 1];

U64 zobrist_piecesquare[32][120];
U64 zobrist_color;
U64 zobrist_castlings[16];
U64 zobrist_en_passant[120];

U64 rand64()
{
    return rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30) ^ ((U64)rand() << 45) ^ ((U64)rand() << 60);
}

void zobrist_init()
{
    for(int i = 0; i < 32; i += 1)
        for(int j = 0; j < 120; j += 1)
            zobrist_piecesquare[i][j] = rand64();
    zobrist_color = rand64();
    for(int i = 0; i < 16; i += 1)
        zobrist_castlings[i] = rand64();
    for(int i = 0; i < 120; i += 1)
        zobrist_en_passant[i] = rand64();
}

#define LESS_THAN_ALPHA 0
#define BETWEEN_ALPHA_AND_BETA 1
#define MORE_THAN_BETA 2

void hash_save_entry(int depth, int eval, Move bestmove, int flag)
{
    Entry *entry = &hash_table[ply->hash & hash_table_size];
    if(!(entry->hash == ply->hash && entry->depth > depth &&
        entry->flag == BETWEEN_ALPHA_AND_BETA))
    {
        entry->hash = ply->hash;
        entry->depth = depth;
        entry->eval = eval;
        entry->bestmove = bestmove;
        entry->flag = flag;
    }
}

Entry* hash_get_entry()
{
    Entry *entry = &hash_table[ply->hash & hash_table_size];
    if(entry->hash == ply->hash)
        return entry;
    return NULL;
}


void setup_hash_and_init_arrays()
{
    clear_history();
    mvv_lva_init();
    zobrist_init();
    
    ply->hash = 0;
    if(turn_to_move == WHITE) ply->hash ^= zobrist_color;
    for(int i = 0; i < 64; i += 1)
    {
        int place = board64[i];
        int figure = board[place];
        if(figure != EMPTY)
            ply->hash ^= zobrist_piecesquare[figure][place];
    }
    ply->hash ^= zobrist_castlings[ply->castlings];
    ply->hash ^= zobrist_en_passant[ply->en_passant];
}

#define start_fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void setup_position(char* fen)
{
    ply = begin_ply;
    turn_to_move = WHITE;
    ply->en_passant = 0;
    ply->castlings = 0;
    ply->number_of_insignificant_plies = 0;
    for(int i = 0; i < 120; i += 1) board[i] = BORDER;
    for(int i = 0; i < 64; i += 1) board[board64[i]] = EMPTY;
    
    int i = 0;
    int current_cell = 0;
    for(; fen[i] != ' ' && fen[i] != '\0'; i += 1, current_cell += 1)
    {
        int index_of_figure = board64[current_cell]; 
        switch(fen[i])
        {
            case '/':
                current_cell -= 1;
            break; case 'K':
                place_of_white_king = index_of_figure;
                board[index_of_figure] = create_figure(WHITE, KING);
            break; case 'Q':
                board[index_of_figure] = create_figure(WHITE, QUEEN);
            break; case 'R':
                board[index_of_figure] = create_figure(WHITE, ROOK);
            break; case 'B':
                board[index_of_figure] = create_figure(WHITE, BISHOP);
            break; case 'N':
                board[index_of_figure] = create_figure(WHITE, KNIGHT);
            break; case 'P':
                board[index_of_figure] = create_figure(WHITE, PAWN);
            break; case 'k':
                place_of_black_king = index_of_figure;
                board[index_of_figure] = create_figure(BLACK, KING);
            break; case 'q':
                board[index_of_figure] = create_figure(BLACK, QUEEN);
            break; case 'r':
                board[index_of_figure] = create_figure(BLACK, ROOK);
            break; case 'b':
                board[index_of_figure] = create_figure(BLACK, BISHOP);
            break; case 'n':
                board[index_of_figure] = create_figure(BLACK, KNIGHT);
            break; case 'p':
                board[index_of_figure] = create_figure(BLACK, PAWN);
            break; default:
                current_cell += fen[i] - '0' - 1;
        }
    }
    if(fen[i] == '\0')
    {
        setup_hash_and_init_arrays();
        return;
    }
    i += 1;
    
    if(fen[i] == 'w')      turn_to_move = WHITE;
    else if(fen[i] == 'b') turn_to_move = BLACK;
    i += 1;
    if(fen[i] == '\0')
    {
        setup_hash_and_init_arrays();
        return;
    }
    i += 1;
    
    for(; fen[i] != ' ' && fen[i] != '\0'; i += 1)
    {
        switch(fen[i])
        {
            case 'K': ply->castlings |= K_castling; break;
            case 'Q': ply->castlings |= Q_castling; break;
            case 'k': ply->castlings |= k_castling; break;
            case 'q': ply->castlings |= q_castling; break;
        }
    }
    if(fen[i] == '\0')
    {
        setup_hash_and_init_arrays();
        return;
    }
    i += 1;
    
    
    if(fen[i] == '-') i += 1;
    else
    {
        ply->en_passant = ('8' - fen[i + 1] + 2) * 10 + fen[i] - 'a' + 1;
        i += 2;
    }
    if(fen[i] == '\0')
    {
        setup_hash_and_init_arrays();
        return;
    }
    i += 1;
    
    if(fen[i + 1] == ' ' || fen[i + 1] == '\0')
        ply->number_of_insignificant_plies = fen[i] - '0';
    else
        ply->number_of_insignificant_plies = ((fen[i] - '0') * 10 + fen[i + 1] - '0');
    
    setup_hash_and_init_arrays();
}

void make_null_move()
{
    turn_to_move = not_turn_to_move;
    ply += 1;
    ply->castlings = (ply - 1)->castlings;
    ply->en_passant = 0;
    ply->number_of_insignificant_plies = (ply - 1)->number_of_insignificant_plies;
    ply->hash = (ply - 1)->hash ^ zobrist_color;
    if((ply - 1)->en_passant)
        ply->hash ^= zobrist_en_passant[(ply - 1)->en_passant];
}

void unmake_null_move()
{
    turn_to_move = not_turn_to_move;
    ply -= 1;
}

int in_check(int turn_to_move)
{
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
        
    for(int i = 0; i < 2; i += 1)
    {
        int tmp = place_of_king + captures_of_pawns[i];
        if(board[tmp] == create_figure(not_turn_to_move, PAWN))
            return 1;
    }
    for(int i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_king[i];
        if(board[tmp] == create_figure(not_turn_to_move, KING))
            return 1;
    }
    for(int i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_knight[i];
        if(board[tmp] == create_figure(not_turn_to_move, KNIGHT))
            return 1;
    }
    for(int i = 0; i < 4; i += 1)
    {
        int inc = directions_of_bishop[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, BISHOP))
                return 1;
    }
    for(int i = 0; i < 4; i += 1)
    {
        int inc = directions_of_rook[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, ROOK))
                return 1;
    }
    return 0;
}

void make_move(Move move)
{
    U64 hash = ply->hash;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int from = move_from(move);
    int figure = board[from];
    int to = move_to(move);
    int turn = move_turn(move);
    int broken = move_broken(move);
    board[from] = EMPTY;
    hash ^= zobrist_piecesquare[figure][from];
    if(turn)
    {
        board[to] = turn;
        hash ^= zobrist_piecesquare[turn][to];
    }
    else
    {
        board[to] = figure;
        hash ^= zobrist_piecesquare[figure][to];
    }
    if(get_value(figure) == PAWN && to == ply->en_passant)
    {
        int tmp = to - direction_of_pawns;
        board[tmp] = EMPTY;
        hash ^= zobrist_piecesquare[create_figure(not_turn_to_move, PAWN)][tmp];
    }
    else if(broken)
    {
        hash ^= zobrist_piecesquare[broken][to];
    }
    if(ply->en_passant)
    {
        hash ^= zobrist_en_passant[ply->en_passant];
    }
    
    ply += 1;
    
    if(get_value(figure) == PAWN && to - from == direction_of_pawns * 2)
    {
        ply->en_passant = from + direction_of_pawns;
        hash ^= zobrist_en_passant[ply->en_passant];
    }
    else
    {
        ply->en_passant = 0;
    }
    
    ply->castlings = (ply - 1)->castlings;
    if(get_value(figure) == KING)
    {
        int horizontal;
        if(turn_to_move == WHITE)
        {
            place_of_white_king = to;
            make_white_castlings_is_incorrect();
            horizontal = 90;
        }
        else
        {
            place_of_black_king = to;
            make_black_castlings_is_incorrect();
            horizontal = 20;
        }
        if(from - to == 2)
        {
            board[horizontal + 4] = board[horizontal + 1];
            board[horizontal + 1] = EMPTY;
        }
        else if(to - from == 2)
        {
            board[horizontal + 6] = board[horizontal + 8];
            board[horizontal + 8] = EMPTY;
        }
    }
    if(board[98] != create_figure(WHITE, ROOK))
        make_K_castling_is_incorrect();
    if(board[91] != create_figure(WHITE, ROOK))
        make_Q_castling_is_incorrect();
    if(board[28] != create_figure(BLACK, ROOK))
        make_k_castling_is_incorrect();
    if(board[21] != create_figure(BLACK, ROOK))
        make_q_castling_is_incorrect();
    
    if(broken || get_value(figure) == PAWN)
        ply->number_of_insignificant_plies = 0;
    else
        ply->number_of_insignificant_plies = 
            (ply - 1)->number_of_insignificant_plies + 1;
    turn_to_move = not_turn_to_move;
    
    hash ^= zobrist_color;
    hash ^= zobrist_castlings[(ply - 1)->castlings];
    hash ^= zobrist_castlings[ply->castlings];
    ply->hash = hash;
}

void unmake_move(Move move)
{
    turn_to_move = not_turn_to_move;
    ply -= 1;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int from = move_from(move);
    int to = move_to(move);
    int figure = board[move_to(move)];
    int broken = move_broken(move);
    int turn = move_turn(move);

    if(turn)
        board[from] = create_figure(turn_to_move, PAWN);
    else
        board[from] = figure;
    
    if(get_value(figure) == PAWN && to == ply->en_passant)
    {
        board[to] = EMPTY;
        board[to - direction_of_pawns] = create_figure(not_turn_to_move, PAWN);
    }
    else
        board[to] = broken;
    
    if(get_value(figure) == KING)
    {
        int horizontal;
        if(turn_to_move == WHITE)
        {
            place_of_white_king = from;
            horizontal = 90;
        }
        else
        {
            place_of_black_king = from;
            horizontal = 20;
        }
        if(from - to == 2)
        {
            board[horizontal + 1] = board[horizontal + 4];
            board[horizontal + 4] = EMPTY;
        }
        else if(to - from == 2)
        {
            board[horizontal + 8] = board[horizontal + 6];
            board[horizontal + 6] = EMPTY;
        }
    }
}

int generate_moves(Move *movelist)
{
    int horizontal2 = turn_to_move == WHITE? 8 : 3;
    int horizontal7 = turn_to_move == WHITE? 3 : 8;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
    int n = 0;
    
    int king_castling, queen_castling;
    if(turn_to_move == WHITE)
    {
        king_castling  = ply->castlings & K_castling;
        queen_castling = ply->castlings & Q_castling;
    }
    else
    {
        king_castling  = ply->castlings & k_castling;
        queen_castling = ply->castlings & q_castling;
    }
    int is_in_check = in_check(turn_to_move);
    for(int i = 0; i < 8; i += 1)
    {
        int i_move = moves_of_king[i];
        int tmp = place_of_king + i_move;
        if(board[tmp] == EMPTY || get_color(board[tmp]) == not_turn_to_move)
        {
            int i_move_is_possible = 0;
            Move tmp_move = create_move(place_of_king, tmp, board[tmp], 0);
            make_move(tmp_move);
            if(!in_check(not_turn_to_move))
            {
                i_move_is_possible = 1;
                movelist[n] = tmp_move;
                n += 1;
            }
            unmake_move(tmp_move);
            
            if(is_in_check || !i_move_is_possible || board[tmp] != EMPTY)
                continue;
            if(i_move == 1 && king_castling && board[place_of_king + 2] == EMPTY)
            {
                movelist[n] = create_move(place_of_king, place_of_king + 2, 0, 0);
                n += 1;
            }
            if(i_move == -1 && queen_castling &&
               board[place_of_king - 2] == EMPTY &&
               board[place_of_king - 3] == EMPTY)
            {
                movelist[n] = create_move(place_of_king, place_of_king - 2, 0, 0);
                n += 1;
            }
        }
    }
    
    if(ply->en_passant)
    {
        for(int i = 0; i < 2; i += 1)
        {
            int tmp = ply->en_passant - captures_of_pawns[i];
            if(board[tmp] == create_figure(turn_to_move, PAWN))
            {
                movelist[n] = create_move(tmp, ply->en_passant, 0, 0);
                n += 1;
            }
        }
    }
    for(int i64 = 0; i64 < 64; i64 += 1)
    {
        int current_cell = board64[i64];
        int figure = board[current_cell];
        if(get_color(figure) != turn_to_move)
            continue;
        switch(get_value(figure))
        {
            case QUEEN:
                for(int i = 0; i < 8; i += 1)
                {
                    int inc = directions_of_queen[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        movelist[n] = create_move(current_cell, x, 0, 0);
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move )
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case ROOK:
                for(int i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_rook[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        movelist[n] = create_move(current_cell, x, 0, 0);
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case BISHOP:
                for(int i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_bishop[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        movelist[n] = create_move(current_cell, x, 0, 0);
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case KNIGHT:
                for(int i = 0; i < 8; i += 1)
                {
                    int tmp = current_cell + moves_of_knight[i];
                    if(board[tmp] == EMPTY ||
                        get_color(board[tmp]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, tmp, board[tmp], 0);
                        n += 1;
                    }
                }
                break;
            case PAWN:
                ;int tmp = current_cell + direction_of_pawns;
                if(board[tmp] == EMPTY)
                {
                    if(current_cell/10 == horizontal7)
                    {
                        for(int j = 0; j < 4; j += 1)
                        {
                            movelist[n] = create_move(current_cell, tmp, 0,
                                create_figure(turn_to_move, turn_figures[j]));
                            n += 1;
                        }
                    }
                    else
                    {
                        movelist[n] = create_move(current_cell, tmp, 0, 0);
                        n += 1;
                    }
                    tmp += direction_of_pawns;
                    if(board[tmp] == EMPTY && current_cell/10 == horizontal2)
                    {
                        movelist[n] = create_move(current_cell, tmp, 0, 0);
                        n += 1;
                    }
                }
                for(int i = 0; i < 2; i += 1)
                {
                    int tmp = current_cell + captures_of_pawns[i];
                    if(get_color(board[tmp]) == not_turn_to_move)
                    {
                        if(current_cell/10 == horizontal7)
                        {
                            for(int j = 0; j < 4; j += 1)
                            {
                                movelist[n] = create_move(current_cell, tmp, board[tmp],
                                    create_figure(turn_to_move, turn_figures[j]));
                                n += 1;
                            }
                        }
                        else
                        {
                            movelist[n] = create_move(current_cell, tmp, board[tmp], 0);
                            n += 1;
                        }
                    }
                }
        }
    }
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        if(in_check(not_turn_to_move))
        {
            movelist[i] = movelist[n - 1];
            n -= 1;
            i -= 1;
        }
        unmake_move(i_move);
    }
    return n;
}

int generate_captures(Move *movelist)
{
    int horizontal2 = turn_to_move == WHITE? 8 : 3;
    int horizontal7 = turn_to_move == WHITE? 3 : 8;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
    int n = 0;
    
    for(int i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_king[i];
        if(get_color(board[tmp]) == not_turn_to_move)
        {
            movelist[n] = create_move(place_of_king, tmp, board[tmp], 0);
            n += 1;
        }
    }
    for(int i64 = 0; i64 < 64; i64 += 1)
    {
        int current_cell = board64[i64];
        int figure = board[current_cell];
        if(get_color(figure) != turn_to_move)
            continue;
        switch(get_value(figure))
        {
            case QUEEN:
                for(int i = 0; i < 8; i += 1)
                {
                    int inc = directions_of_queen[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case ROOK:
                for(int i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_rook[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case BISHOP:
                for(int i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_bishop[i];
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, x, board[x], 0);
                        n += 1;
                    }
                }
                break;
            case KNIGHT:
                for(int i = 0; i < 8; i += 1)
                {
                    int tmp = current_cell + moves_of_knight[i];
                    if(get_color(board[tmp]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, tmp, board[tmp], 0);
                        n += 1;
                    }
                }
                break;
            case PAWN:
                for(int i = 0; i < 2; i += 1)
                {
                    int tmp = current_cell + captures_of_pawns[i];
                    if(get_color(board[tmp]) == not_turn_to_move)
                    {
                        movelist[n] = create_move(current_cell, tmp, board[tmp], 0);
                        n += 1;
                    }
                }
        }
    }
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        if(in_check(not_turn_to_move))
        {
            movelist[i] = movelist[n - 1];
            n -= 1;
            i -= 1;
        }
        unmake_move(i_move);
    }
    return n;
}

U64 perft(int depth)
{
    Move movelist[256];
    int n;
    U64 result = 0;
    
    if(depth == 0) return 1;
    
    n = generate_moves(movelist);
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        result += perft(depth - 1);
        unmake_move(i_move);
    }
    return result;
}

#define check_perft_on_position(depth, fen) \
setup_position(fen);\
printf("%lld\n", perft(depth))




//Now all piece-square tables and material are from
//  http://chessprogramming.wikispaces.com/Simplified+evaluation+function
//Material Q: 900, R: 500, B: 330, N: 320, P: 100

int PST_W_KING[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};
int PST_W_QUEEN[64] = {
    880,890,890,895,895,890,890,880,
    890,900,900,900,900,900,900,890,
    890,900,905,905,905,905,900,890,
    895,900,905,905,905,905,900,895,
    900,900,905,905,905,905,900,895,
    890,905,905,905,905,905,900,890,
    890,900,905,900,900,900,900,890,
    880,890,890,895,895,890,890,880
};
int PST_W_ROOK[64] = {
    500,500,500,500,500,500,500,500,
    505,510,510,510,510,510,510,505,
    495,500,500,500,500,500,500,495,
    495,500,500,500,500,500,500,495,
    495,500,500,500,500,500,500,495,
    495,500,500,500,500,500,500,495,
    495,500,500,500,500,500,500,495,
    500,500,500,505,505,500,500,500
};
int PST_W_BISHOP[64] = {
    310,320,320,320,320,320,320,310,
    320,330,330,330,330,330,330,320,
    320,330,335,340,340,335,330,320,
    320,335,335,340,340,335,335,320,
    320,330,340,340,340,340,330,320,
    320,340,340,340,340,340,340,320,
    320,335,330,330,330,330,335,320,
    310,320,320,320,320,320,320,310
};
int PST_W_KNIGHT[64] = {
    270,280,290,290,290,290,280,270,
    280,300,320,320,320,320,300,280,
    290,320,330,335,335,330,320,290,
    290,325,335,340,340,335,325,290,
    290,320,335,340,340,335,320,290,
    290,325,330,335,335,330,325,290,
    280,300,320,325,325,320,300,280,
    270,280,290,290,290,290,280,270
};
int PST_W_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
    150,150,150,150,150,150,150,150,
    110,110,120,130,130,120,110,110,
    105,105,110,125,125,110,105,105,
    100,100,100,120,120,100,100,100,
    105, 95, 90,100,100, 90, 95,105,
    105,110,110, 80, 80,110,110,105,
      0,  0,  0,  0,  0,  0,  0,  0,
};
int PST_B_KING[64] = {
   -20,-30,-10,  0,  0,-10,-30,-20,
   -20,-20,  0,  0,  0,  0,-20,-20,
    10, 20, 20, 20, 20, 20, 20, 10,
    20, 30, 30, 40, 40, 30, 30, 20,
    30, 40, 40, 50, 50, 40, 40, 30,
    30, 40, 40, 50, 50, 40, 40, 30,
    30, 40, 40, 50, 50, 40, 40, 30,
    30, 40, 40, 50, 50, 40, 40, 30
};
int PST_B_QUEEN[64] = {
    -880,-890,-890,-895,-895,-890,-890,-880,
    -890,-900,-900,-900,-900,-905,-900,-890,
    -890,-900,-905,-905,-905,-905,-905,-890,
    -895,-900,-905,-905,-905,-905,-900,-900,
    -895,-900,-905,-905,-905,-905,-900,-895,
    -890,-900,-905,-905,-905,-905,-900,-890,
    -890,-900,-900,-900,-900,-900,-900,-890,
    -880,-890,-890,-895,-895,-890,-890,-880
};
int PST_B_ROOK[64] = {
    -500,-500,-500,-505,-505,-500,-500,-500,
    -495,-500,-500,-500,-500,-500,-500,-495,
    -495,-500,-500,-500,-500,-500,-500,-495,
    -495,-500,-500,-500,-500,-500,-500,-495,
    -495,-500,-500,-500,-500,-500,-500,-495,
    -495,-500,-500,-500,-500,-500,-500,-495,
    -505,-510,-510,-510,-510,-510,-510,-505,
    -500,-500,-500,-500,-500,-500,-500,-500
};
int PST_B_BISHOP[64] = {
    -310,-320,-320,-320,-320,-320,-320,-310,
    -320,-335,-330,-330,-330,-330,-335,-320,
    -320,-340,-340,-340,-340,-340,-340,-320,
    -320,-330,-340,-340,-340,-340,-330,-320,
    -320,-335,-335,-340,-340,-335,-335,-320,
    -320,-330,-335,-340,-340,-335,-330,-320,
    -320,-330,-330,-330,-330,-330,-330,-320,
    -310,-320,-320,-320,-320,-320,-320,-310
};
int PST_B_KNIGHT[64] = {
    -270,-280,-290,-290,-290,-290,-280,-270,
    -280,-300,-320,-325,-325,-320,-300,-280,
    -290,-325,-330,-335,-335,-330,-325,-290,
    -290,-320,-335,-340,-340,-335,-320,-290,
    -290,-325,-335,-340,-340,-335,-325,-290,
    -290,-320,-330,-335,-335,-330,-320,-290,
    -280,-300,-320,-320,-320,-320,-300,-280,
    -270,-280,-290,-290,-290,-290,-280,-270
};
int PST_B_PAWN[64] = {
       0,   0,   0,   0,   0,   0,   0,   0,
    -105,-110,-110, -80, -80,-110,-110,-105,
    -105, -95, -90,-100,-100, -90, -95,-105,
    -100,-100,-100,-120,-120,-100,-100,-100,
    -105,-105,-110,-125,-125,-110,-105,-105,
    -110,-110,-120,-130,-130,-120,-110,-110,
    -150,-150,-150,-150,-150,-150,-150,-150,
       0,   0,   0,   0,   0,   0,   0,   0
};

int evaluate(int alpha, int beta)
{
    Entry *entry = hash_get_entry();
    if(entry != NULL)
    {
        int eval = entry->eval, flag = entry->flag;
        if(flag != LESS_THAN_ALPHA && eval >= beta)
            return beta;
        if(flag != MORE_THAN_BETA && eval <= alpha)
            return alpha;
        if(flag == BETWEEN_ALPHA_AND_BETA)
            return eval;
    }
    
    int evaluation = 0;
    for(int i = 0; i < 64; i += 1)
    {
        switch(board[board64[i]])
        {
            case create_figure(WHITE, KING):
                evaluation += PST_W_KING[i];
                break;
            case create_figure(WHITE, QUEEN):
                evaluation += PST_W_QUEEN[i];
                break;
            case create_figure(WHITE, ROOK):
                evaluation += PST_W_ROOK[i];
                break;
            case create_figure(WHITE, BISHOP):
                evaluation += PST_W_BISHOP[i];
                break;
            case create_figure(WHITE, KNIGHT):
                evaluation += PST_W_KNIGHT[i];
                break;
            case create_figure(WHITE, PAWN):
                evaluation += PST_W_PAWN[i];
                break;
            case create_figure(BLACK, KING):
                evaluation += PST_B_KING[i];
                break;
            case create_figure(BLACK, QUEEN):
                evaluation += PST_B_QUEEN[i];
                break;
            case create_figure(BLACK, ROOK):
                evaluation += PST_B_ROOK[i];
                break;
            case create_figure(BLACK, BISHOP):
                evaluation += PST_B_BISHOP[i];
                break;
            case create_figure(BLACK, KNIGHT):
                evaluation += PST_B_KNIGHT[i];
                break;
            case create_figure(BLACK, PAWN):
                evaluation += PST_B_PAWN[i];
                break;
        }
    }
    return turn_to_move == WHITE? evaluation: -evaluation;
}


void sorting_captures(Move *movelist, int n)
{
    int sorting_values[n];
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        int figure = board[move_from(i_move)];
        int broken = move_broken(i_move);
        sorting_values[i] = mvv_lva[figure][broken];
    }
    for(int i = 1; i < n; i += 1)
    {
        int j = i;
        while(sorting_values[j] > sorting_values[j - 1] && j > 0)
        {
            int tmp = sorting_values[j];
            sorting_values[j] = sorting_values[j - 1];
            sorting_values[j - 1] = tmp;
            
            Move tmp2 = movelist[j];
            movelist[j] = movelist[j - 1];
            movelist[j - 1] = tmp2;
            j -= 1;
        }
    }
}

void sorting_moves(Move *movelist, int n)
{
    int sorting_values[n];
    Entry *entry = hash_get_entry();
    Move hash_move = 0;
    if(entry != NULL)
        hash_move = entry->bestmove;
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        if(i_move == hash_move)
        {
            sorting_values[i] = 10000000;
        }
        else if(move_broken(i_move))
        {
            int figure = board[move_from(i_move)];
            int broken = move_broken(i_move);
            sorting_values[i] = mvv_lva[figure][broken];
        }
        else
        {
            sorting_values[i] = history[board[move_from(i_move)]][move_to(i_move)];
        }
    }
    for(int i = 1; i < n; i += 1)
    {
        int j = i;
        while(sorting_values[j] > sorting_values[j - 1] && j > 0)
        {
            int tmp = sorting_values[j];
            sorting_values[j] = sorting_values[j - 1];
            sorting_values[j - 1] = tmp;
            
            Move tmp2 = movelist[j];
            movelist[j] = movelist[j - 1];
            movelist[j - 1] = tmp2;
            j -= 1;
        }
    }
}

int is_draw_by_repetition_or_50_moves()
{
    int number_of_insignificant_plies = ply->number_of_insignificant_plies;
    if(number_of_insignificant_plies == 100)
    {
        return 1;
    }
    int number_of_repetitions = 1;
    U64 hash = ply->hash;
    for(int i = 1; i < number_of_insignificant_plies && (ply - i) >= begin_ply; i += 1)
    {
        if(hash == (ply - i)->hash)
        {
            number_of_repetitions += 1;
            if(number_of_repetitions == 3)
            {
                return 1;
            }
        }
    }
    return 0;
}

int quiescence(int alpha, int beta)
{
    int static_evaluation = evaluate(alpha, beta);
    if(static_evaluation >= beta)
        return beta;
    if(static_evaluation > alpha)
        alpha = static_evaluation;
    
    Move movelist[256];
    int n = generate_captures(movelist);
    sorting_captures(movelist, n);
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        int score = -quiescence(-beta, -alpha);
        unmake_move(i_move);
        
        if(score >= beta)
        {
            return beta;
        }
        if(score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

int ZWS(int beta, int depth, int can_null)
{
    if(is_draw_by_repetition_or_50_moves()) return DRAW;
    
    Entry *entry = hash_get_entry();
    if(entry != NULL && entry->depth >= depth)
    {
        int eval = entry->eval, flag = entry->flag;
        if(flag != LESS_THAN_ALPHA && eval >= beta)
            return beta;
        if(flag != MORE_THAN_BETA && eval < beta)
            return beta - 1;
    }
    
    if(depth == 0) return quiescence(beta - 1, beta);
    
    if(depth <= 6 && evaluate(beta - 1, beta) >= beta)
    {
        return beta;
    }
    
    if(depth > 2 && can_null && !in_check(turn_to_move))
    {
        int R = depth > 6 ? 3: 2;
        make_null_move();
        int score = -ZWS(-beta + 1, depth - R - 1, 0);
        unmake_null_move();
        if(score >= beta) return beta;
    }
    
    Move movelist[256];
    int n = generate_moves(movelist);
    if(n == 0)
    {
        if(!in_check(turn_to_move))
            return DRAW;
        return LOSING + ply - begin_ply;
    }
    sorting_moves(movelist, n);
    
    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        int score = -ZWS(-beta + 1, depth - 1, can_null);
        unmake_move(i_move);
        
        if(score >= beta)
        {
            hash_save_entry(depth, beta, i_move, MORE_THAN_BETA);
            return beta;
        }
    }
    hash_save_entry(depth, beta - 1, 0, LESS_THAN_ALPHA);
    return beta - 1;
}

int PVS(int alpha, int beta, int depth)
{
    if(is_draw_by_repetition_or_50_moves()) return DRAW;
    
    if(depth == 0) return quiescence(alpha, beta);
    
    Move movelist[256];
    int n = generate_moves(movelist);
    if(n == 0)
    {
        if(!in_check(turn_to_move))
            return DRAW;
        return LOSING + ply - begin_ply;
    }
    sorting_moves(movelist, n);
    
    int bool_search_pv = 1;
    Move bestmove = 0;

    for(int i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        int score;
        if(bool_search_pv)
        {
            score = -PVS(-beta, -alpha, depth - 1);
        }
        else
        {
            score = -ZWS(-alpha, depth - 1, 1);
            if(score > alpha)
                score = -PVS(-beta, -alpha, depth - 1);
        }
        unmake_move(i_move);
        
        if(score >= beta)
        {
            hash_save_entry(depth, beta, i_move, MORE_THAN_BETA);
            if(!move_broken(i_move))
            {
                history[board[move_from(i_move)]][move_to(i_move)] = depth * depth;
            }
            return beta;
        }
        if(score > alpha)
        {
            bestmove = i_move;
            alpha = score;
        }
        bool_search_pv = 0;
    }
    if(bestmove != 0)
        hash_save_entry(depth, alpha, bestmove, BETWEEN_ALPHA_AND_BETA);
    else
        hash_save_entry(depth, alpha, 0, LESS_THAN_ALPHA);
    return alpha;
}

Move search(int depth)
{
    PVS(-1000000, 1000000, depth);
    Entry *entry = hash_get_entry();
    if(entry != NULL) return entry->bestmove;
    return 0;
}


//game with engine

Move parse_move(char *string)
{
    for(int i = 0; i < 4; i += 1)
        if(string[i] == '\0') return 0;
    if(!((string[0] >= 'a' && string[0] <= 'h') &&
         (string[1] >= '1' && string[1] <= '8') &&
         (string[2] >= 'a' && string[2] <= 'h') &&
         (string[3] >= '1' && string[3] <= '8'))) return 0;
    
    int from = ('8' - string[1] + 2) * 10 + (string[0] - 'a' + 1);
    int to   = ('8' - string[3] + 2) * 10 + (string[2] - 'a' + 1);
    int broken = board[to];
    int turn;
    switch(string[4])
    {
        case '\0': turn = 0; break;
        case 'q':  turn = create_figure(turn_to_move, QUEEN); break;
        case 'r':  turn = create_figure(turn_to_move, ROOK); break;
        case 'b':  turn = create_figure(turn_to_move, BISHOP); break;
        case 'n':  turn = create_figure(turn_to_move, KNIGHT); break;
        default: return 0;
    }
    return create_move(from, to, broken, turn);
}

int main()
{
    Move movelist[256];
    int n;
    int default_depth = 12;
    setup_position(start_fen);
    while(1)
    {
        print_position();
        n = generate_moves(movelist);
        int flag = 1;
        while(flag)
        {
            char string[10];
            scanf("%s", string);
            Move move = parse_move(string);
            for(int i = 0; i < n; i += 1)
            {
                Move i_move = movelist[i]; 
                if(move == i_move)
                {
                    make_move(move);
                    flag = 0;
                }
            }
        }
        
        print_position();
        Move bestmove = search(default_depth);
        if(bestmove == 0)
            break;
        make_move(bestmove);
    }
    return 0;
}
