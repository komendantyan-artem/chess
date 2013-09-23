#include <stdio.h>

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
} begin_ply[1000], *ply;

void print_position()
{
    int i, j;
    for(i = 0; i < 8; i += 1)
    {
        for(j = 0; j < 8; j += 1)
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

#define start_fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void setup_position(char* fen)
{
    int i, current_cell;
    ply = begin_ply;
    turn_to_move = WHITE;
    ply->en_passant = 0;
    ply->castlings = 0;
    ply->number_of_insignificant_plies = 0;
    for(i = 0; i < 120; i += 1) board[i] = BORDER;
    for(i = 0; i < 64; i += 1) board[board64[i]] = EMPTY;
    
    i = 0;
    current_cell = 0;
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
    if(fen[i] == '\0') return;
    i += 1;
    
    if(fen[i] == 'w')      turn_to_move = WHITE;
    else if(fen[i] == 'b') turn_to_move = BLACK;
    i += 1;
    if(fen[i] == '\0') return;
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
    if(fen[i] == '\0') return;
    i += 1;
    
    
    if(fen[i] == '-') i += 1;
    else
    {
        ply->en_passant = ('8' - fen[i + 1] + 2) * 10 + fen[i] - 'a' + 1;
        i += 2;
    }
    if(fen[i] == '\0') return;
    i += 1;
    
    if(fen[i + 1] == ' ' || fen[i + 1] == '\0')
        ply->number_of_insignificant_plies = fen[i] - '0';
    else
        ply->number_of_insignificant_plies = ((fen[i] - '0') * 10 + 
                                                         fen[i + 1] - '0');
    //Number of moves is not used.
}


typedef struct _move {
    int from;
    int to;
    int broken;
    int turn;
} Move;

void make_move(Move move)
{
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int figure = board[move.from];
    board[move.from] = EMPTY;
    board[move.to] = figure;
    if(move.turn)
        board[move.to] = move.turn;
    if(get_value(figure) == PAWN && move.to == ply->en_passant)
        board[move.to - direction_of_pawns] = EMPTY;
    ply += 1;
    if(get_value(figure) == PAWN &&
       move.to - move.from == direction_of_pawns*2)
            ply->en_passant = move.from + direction_of_pawns;
    else
        ply->en_passant = 0;
    
    ply->castlings = (ply - 1)->castlings;
    if(get_value(figure) == KING)
    {
        int horizontal;
        if(turn_to_move == WHITE)
        {
            place_of_white_king = move.to;
            make_white_castlings_is_incorrect();
            horizontal = 90;
        }
        else
        {
            place_of_black_king = move.to;
            make_black_castlings_is_incorrect();
            horizontal = 20;
        }
        if(move.from - move.to == 2)
        {
            board[horizontal + 4] = board[horizontal + 1];
            board[horizontal + 1] = EMPTY;
        }
        else if(move.to - move.from == 2)
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
    
    if(move.broken || get_value(figure) == PAWN)
        ply->number_of_insignificant_plies = 0;
    else
        ply->number_of_insignificant_plies = 
            (ply - 1)->number_of_insignificant_plies + 1;
    turn_to_move = not_turn_to_move;
}

void unmake_move(Move move)
{
    turn_to_move = not_turn_to_move;
    ply -= 1;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int figure = board[move.to];

    if(move.turn)
        board[move.from] = create_figure(turn_to_move, PAWN);
    else
        board[move.from] = figure;
    
    if(get_value(figure) == PAWN && move.to == ply->en_passant)
    {
        board[move.to] = EMPTY;
        board[move.to - direction_of_pawns] =
            create_figure(not_turn_to_move, PAWN);
    }
    else
        board[move.to] = move.broken;
    
    if(get_value(figure) == KING)
    {
        int horizontal;
        if(turn_to_move == WHITE)
        {
            place_of_white_king = move.from;
            horizontal = 90;
        }
        else
        {
            place_of_black_king = move.from;
            horizontal = 20;
        }
        if(move.from - move.to == 2)
        {
            board[horizontal + 1] = board[horizontal + 4];
            board[horizontal + 4] = EMPTY;
        }
        else if(move.to - move.from == 2)
        {
            board[horizontal + 8] = board[horizontal + 6];
            board[horizontal + 6] = EMPTY;
        }
    }
}



int get_rentgen_and_atackers
    (int defend_against_check[120], int rentgen[120][2])
{
    int i;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
    int number_of_atackers = 0;
    for(i = 0; i < 120; i += 1)
        defend_against_check[i] = rentgen[i][0] = rentgen[i][1] = 0;
        
    for(i = 0; i < 2; i += 1)
    {
        int tmp = place_of_king + captures_of_pawns[i];
        if(board[tmp] == create_figure(not_turn_to_move, PAWN))
        {
            number_of_atackers += 1;
            defend_against_check[tmp] = 1;
        }
    }
    for(i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_knight[i];
        if(board[tmp] == create_figure(not_turn_to_move, KNIGHT))
        {
            number_of_atackers += 1;
            defend_against_check[tmp] = 1;
        }
    }
    for(i = 0; i < 4; i += 1)
    {
        int inc = directions_of_bishop[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, BISHOP))
        {
            number_of_atackers += 1;
            while(x != place_of_king)
            {
                defend_against_check[x] = 1;
                x -= inc;
            }
        }
        else if(get_color(board[x]) == turn_to_move)
        {
            int tmp = x;
            x += inc;
            while(board[x] == EMPTY)
                x += inc;
            if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
               board[x] == create_figure(not_turn_to_move, BISHOP))
            {
                rentgen[tmp][0] = 1;
                rentgen[tmp][1] = inc;
            }
        }
    }
    for(i = 0; i < 4; i += 1)
    {
        int inc = directions_of_rook[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, ROOK))
        {
            number_of_atackers += 1;
            while(x != place_of_king)
            {
                defend_against_check[x] = 1;
                x -= inc;
            }
        }
        else if(get_color(board[x]) == turn_to_move)
        {
            int tmp = x;
            x += inc;
            while(board[x] == EMPTY)
                x += inc;
            if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
               board[x] == create_figure(not_turn_to_move, ROOK))
            {
                rentgen[tmp][0] = 1;
                rentgen[tmp][1] = inc;
            }
        }
    }
    return number_of_atackers;
}

int not_in_check(int turn_to_move)
{
    int i;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
        
    for(i = 0; i < 2; i += 1)
    {
        int tmp = place_of_king + captures_of_pawns[i];
        if(board[tmp] == create_figure(not_turn_to_move, PAWN))
            return 0;
    }
    for(i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_king[i];
        if(board[tmp] == create_figure(not_turn_to_move, KING))
            return 0;
    }
    for(i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_knight[i];
        if(board[tmp] == create_figure(not_turn_to_move, KNIGHT))
            return 0;
    }
    for(i = 0; i < 4; i += 1)
    {
        int inc = directions_of_bishop[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, BISHOP))
                return 0;
    }
    for(i = 0; i < 4; i += 1)
    {
        int inc = directions_of_rook[i];
        int x = place_of_king + inc;
        while(board[x] == EMPTY)
            x += inc;
        if(board[x] == create_figure(not_turn_to_move, QUEEN) ||
           board[x] == create_figure(not_turn_to_move, ROOK))
                return 0;
    }
    return 1;
}

int generate_moves(Move *movelist)
{
    int i;
    
    int horizontal2 = turn_to_move == WHITE? 8 : 3;
    int horizontal7 = turn_to_move == WHITE? 3 : 8;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
    int n = 0;
                                    

    int defend_against_check[120], rentgen[120][2];
    int number_of_atackers =
        get_rentgen_and_atackers(defend_against_check, rentgen);
    
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
    for(i = 0; i < 8; i += 1)
    {
        int i_move = moves_of_king[i];
        int tmp = place_of_king + i_move;
        if(board[tmp] == EMPTY ||
           get_color(board[tmp]) == not_turn_to_move)
        {
            int i_move_is_possible = 0;
            Move tmp_move =
                {.from = place_of_king, .to = tmp, .broken = board[tmp]};
            make_move(tmp_move);
            if(not_in_check(not_turn_to_move))
            {
                i_move_is_possible = 1;
                movelist[n] = tmp_move;
                n += 1;
            }
            unmake_move(tmp_move);
            
            if(number_of_atackers || !i_move_is_possible || board[tmp] != EMPTY)
                continue;
            if(i_move == 1 && king_castling &&
               board[place_of_king + 2] == EMPTY)
            {
                Move tmp_move = {.from = place_of_king,
                                 .to = place_of_king + 2};
                make_move(tmp_move);
                if(not_in_check(not_turn_to_move))
                {
                    movelist[n] = tmp_move;
                    n += 1;
                }
                unmake_move(tmp_move);
            }
            if(i_move == -1 && queen_castling &&
               board[place_of_king - 2] == EMPTY &&
               board[place_of_king - 3] == EMPTY)
            {
                Move tmp_move = {.from = place_of_king,
                                 .to = place_of_king - 2};
                make_move(tmp_move);
                if(not_in_check(not_turn_to_move))
                {
                    movelist[n] = tmp_move;
                    n += 1;
                }
                unmake_move(tmp_move);
            }
        }
    }
    if(number_of_atackers == 2)
        return n;
    
    if(ply->en_passant)
    {
        for(i = 0; i < 2; i += 1)
        {
            int tmp = ply->en_passant - captures_of_pawns[i];
            if(board[tmp] == create_figure(turn_to_move, PAWN))
            {
                Move tmp_move = {.from = tmp, .to = ply->en_passant,
                    .broken = create_figure(not_turn_to_move, PAWN)};
                make_move(tmp_move);
                if(not_in_check(not_turn_to_move))
                {
                    movelist[n] = tmp_move;
                    n += 1;
                }
                unmake_move(tmp_move);
            }
        }
    }
    
    if(number_of_atackers == 1)
    {
        int i64;
        for(i64 = 0; i64 < 64; i64 += 1)
        {
            int current_cell = board64[i64];
            int figure = board[current_cell];
            if(get_color(figure) != turn_to_move || rentgen[current_cell][0])
                continue;
            switch(get_value(figure))
            {
                case QUEEN:
                    for(i = 0; i < 8; i += 1)
                    {
                        int inc = directions_of_queen[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                        {
                            if(defend_against_check[x])
                            {
                                Move tmp_move = {.from = current_cell, .to = x};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                            x += inc;
                        }
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case ROOK:
                    for(i = 0; i < 4; i += 1)
                    {
                        int inc = directions_of_rook[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                        {
                            if(defend_against_check[x])
                            {
                                Move tmp_move = {.from = current_cell, .to = x};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                            x += inc;
                        }
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case BISHOP:
                    for(i = 0; i < 4; i += 1)
                    {
                        int inc = directions_of_bishop[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                        {
                            if(defend_against_check[x])
                            {
                                Move tmp_move = {.from = current_cell, .to = x};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                            x += inc;
                        }
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case KNIGHT:
                    for(i = 0; i < 8; i += 1)
                    {
                        int tmp = current_cell + moves_of_knight[i];
                        if(defend_against_check[tmp] &&
                            (board[tmp] == EMPTY ||
                             get_color(board[tmp]) == not_turn_to_move))
                        {
                            Move tmp_move = {.from = current_cell,
                                .to = tmp, .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case PAWN:
                    ;int tmp = current_cell + direction_of_pawns;
                    if(board[tmp] == EMPTY)
                    {
                        if(defend_against_check[tmp])
                        {
                            if(current_cell/10 == horizontal7)
                            {
                                int j;
                                for(j = 0; j < 4; j += 1)
                                {
                                    Move tmp_move =
                                        {.from = current_cell, .to = tmp,
                                        .turn = create_figure(turn_to_move, 
                                        turn_figures[j])};
                                    movelist[n] = tmp_move;
                                    n += 1;
                                }
                            }
                            else
                            {
                                Move tmp_move = {.from = current_cell, .to = tmp};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                        }
                        tmp += direction_of_pawns;
                        if(board[tmp] == EMPTY &&
                           current_cell/10 == horizontal2 &&
                           defend_against_check[tmp])
                        {
                            Move tmp_move = {.from = current_cell, .to = tmp};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    for(i = 0; i < 2; i += 1)
                    {
                        int tmp = current_cell + captures_of_pawns[i];
                        if(defend_against_check[tmp] &&
                           get_color(board[tmp]) == not_turn_to_move)
                        {
                            if(current_cell/10 == horizontal7)
                            {
                                int j;
                                for(j = 0; j < 4; j += 1)
                                {
                                    Move tmp_move =
                                        {.from = current_cell, .to = tmp,
                                        .broken = board[tmp],
                                        .turn = create_figure(turn_to_move, 
                                        turn_figures[j])};
                                    movelist[n] = tmp_move;
                                    n += 1;
                                }
                            }
                            else
                            {
                                Move tmp_move = {.from = current_cell,
                                    .to = tmp, .broken = board[tmp]};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                        }
                    }
            }
        }
        return n;
    }
    //if(number_of_atackers == 0)
    int i64;
    for(i64 = 0; i64 < 64; i64 += 1)
    {
        int current_cell = board64[i64];
        int figure = board[current_cell];
        if(get_color(figure) != turn_to_move)
            continue;
        switch(get_value(figure))
        {
            case QUEEN:
                for(i = 0; i < 8; i += 1)
                {
                    int inc = directions_of_queen[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        Move tmp_move = {.from = current_cell, .to = x};
                        movelist[n] = tmp_move;
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case ROOK:
                for(i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_rook[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        Move tmp_move = {.from = current_cell, .to = x};
                        movelist[n] = tmp_move;
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case BISHOP:
                for(i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_bishop[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                    {
                        Move tmp_move = {.from = current_cell, .to = x};
                        movelist[n] = tmp_move;
                        n += 1;
                        x += inc;
                    }
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case KNIGHT:
                if(!rentgen[current_cell][0])
                {
                    for(i = 0; i < 8; i += 1)
                    {
                        int tmp = current_cell + moves_of_knight[i];
                        if(board[tmp] == EMPTY ||
                           get_color(board[tmp]) == not_turn_to_move)
                        {
                            Move tmp_move = {.from = current_cell, .to = tmp,
                                .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                }
                break;
            case PAWN:
                ;int tmp = current_cell + direction_of_pawns;
                if(!(rentgen[current_cell][0] &&
                   abs(rentgen[current_cell][1]) != abs(direction_of_pawns)) &&
                   board[tmp] == EMPTY)
                {
                    if(current_cell/10 == horizontal7)
                    {
                        int j;
                        for(j = 0; j < 4; j += 1)
                        {
                            Move tmp_move =
                                {.from = current_cell, .to = tmp,
                                .turn = create_figure(turn_to_move, 
                                turn_figures[j])};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    else
                    {
                        Move tmp_move = {.from = current_cell, .to = tmp};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                    tmp += direction_of_pawns;
                    if(board[tmp] == EMPTY && current_cell/10 == horizontal2)
                    {
                        Move tmp_move = {.from = current_cell, .to = tmp};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                for(i = 0; i < 2; i += 1)
                {
                    int capture = captures_of_pawns[i];
                    int tmp = current_cell + capture;
                    if(!(rentgen[current_cell][0] &&
                         abs(rentgen[current_cell][1]) !=
                             abs(capture)) &&
                         get_color(board[tmp]) == not_turn_to_move)
                    {
                        if(current_cell/10 == horizontal7)
                        {
                            int j;
                            for(j = 0; j < 4; j += 1)
                            {
                                Move tmp_move =
                                    {.from = current_cell, .to = tmp,
                                    .broken = board[tmp],
                                    .turn = create_figure(turn_to_move, 
                                    turn_figures[j])};
                                movelist[n] = tmp_move;
                                n += 1;
                            }
                        }
                        else
                        {
                            Move tmp_move = {.from = current_cell, .to = tmp,
                                .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                }
        }
    }
    return n;
}

int generate_captures(Move *movelist)
{
    int i;
    
    int horizontal2 = turn_to_move == WHITE? 8 : 3;
    int horizontal7 = turn_to_move == WHITE? 3 : 8;
    int direction_of_pawns = turn_to_move == WHITE? -10: 10;
    int captures_of_pawns[2] = {direction_of_pawns + 1, direction_of_pawns - 1};
    int place_of_king = turn_to_move == WHITE? place_of_white_king:
                                               place_of_black_king;
    int n = 0;
                                    

    int defend_against_check[120], rentgen[120][2];
    int number_of_atackers =
        get_rentgen_and_atackers(defend_against_check, rentgen);
    
    for(i = 0; i < 8; i += 1)
    {
        int tmp = place_of_king + moves_of_king[i];
        if(get_color(board[tmp]) == not_turn_to_move)
        {
            Move tmp_move =
                {.from = place_of_king, .to = tmp, .broken = board[tmp]};
            make_move(tmp_move);
            if(not_in_check(not_turn_to_move))
            {
                movelist[n] = tmp_move;
                n += 1;
            }
            unmake_move(tmp_move);
        }
    }
    if(number_of_atackers == 2)
        return n;
    
    if(ply->en_passant)
    {
        for(i = 0; i < 2; i += 1)
        {
            int tmp = ply->en_passant - captures_of_pawns[i];
            if(board[tmp] == create_figure(turn_to_move, PAWN))
            {
                Move tmp_move = {.from = tmp, .to = ply->en_passant,
                    .broken = create_figure(not_turn_to_move, PAWN)};
                make_move(tmp_move);
                if(not_in_check(not_turn_to_move))
                {
                    movelist[n] = tmp_move;
                    n += 1;
                }
                unmake_move(tmp_move);
            }
        }
    }
    
    if(number_of_atackers == 1)
    {
        int i64;
        for(i64 = 0; i64 < 64; i64 += 1)
        {
            int current_cell = board64[i64];
            int figure = board[current_cell];
            if(get_color(figure) != turn_to_move || rentgen[current_cell][0])
                continue;
            switch(get_value(figure))
            {
                case QUEEN:
                    for(i = 0; i < 8; i += 1)
                    {
                        int inc = directions_of_queen[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                            x += inc;
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case ROOK:
                    for(i = 0; i < 4; i += 1)
                    {
                        int inc = directions_of_rook[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                            x += inc;
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case BISHOP:
                    for(i = 0; i < 4; i += 1)
                    {
                        int inc = directions_of_bishop[i];
                        int x = current_cell + inc;
                        while(board[x] == EMPTY)
                            x += inc;
                        if(get_color(board[x]) == not_turn_to_move &&
                           defend_against_check[x])
                        {
                            Move tmp_move = {.from = current_cell, .to = x,
                                .broken = board[x]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case KNIGHT:
                    for(i = 0; i < 8; i += 1)
                    {
                        int tmp = current_cell + moves_of_knight[i];
                        if(defend_against_check[tmp] &&
                           get_color(board[tmp]) == not_turn_to_move)
                        {
                            Move tmp_move = {.from = current_cell,
                                .to = tmp, .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                    break;
                case PAWN:
                    for(i = 0; i < 2; i += 1)
                    {
                        int tmp = current_cell + captures_of_pawns[i];
                        if(defend_against_check[tmp] &&
                           get_color(board[tmp]) == not_turn_to_move)
                        {
                            Move tmp_move = {.from = current_cell,
                                .to = tmp, .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
            }
        }
        return n;
    }
    //if(number_of_atackers == 0)
    int i64;
    for(i64 = 0; i64 < 64; i64 += 1)
    {
        int current_cell = board64[i64];
        int figure = board[current_cell];
        if(get_color(figure) != turn_to_move)
            continue;
        switch(get_value(figure))
        {
            case QUEEN:
                for(i = 0; i < 8; i += 1)
                {
                    int inc = directions_of_queen[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case ROOK:
                for(i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_rook[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case BISHOP:
                for(i = 0; i < 4; i += 1)
                {
                    int inc = directions_of_bishop[i];
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
                    int x = current_cell + inc;
                    while(board[x] == EMPTY)
                        x += inc;
                    if(get_color(board[x]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = x,
                            .broken = board[x]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
                break;
            case KNIGHT:
                if(!rentgen[current_cell][0])
                {
                    for(i = 0; i < 8; i += 1)
                    {
                        int tmp = current_cell + moves_of_knight[i];
                        if(get_color(board[tmp]) == not_turn_to_move)
                        {
                            Move tmp_move = {.from = current_cell, .to = tmp,
                                .broken = board[tmp]};
                            movelist[n] = tmp_move;
                            n += 1;
                        }
                    }
                }
                break;
            case PAWN:
                for(i = 0; i < 2; i += 1)
                {
                    int capture = captures_of_pawns[i];
                    int tmp = current_cell + capture;
                    if(!(rentgen[current_cell][0] &&
                         abs(rentgen[current_cell][1]) !=
                             abs(capture)) &&
                         get_color(board[tmp]) == not_turn_to_move)
                    {
                        Move tmp_move = {.from = current_cell, .to = tmp,
                            .broken = board[tmp]};
                        movelist[n] = tmp_move;
                        n += 1;
                    }
                }
        }
    }
    return n;
}

int perft(depth)
{
    Move movelist[256];
    int n;
    int i;
    int result = 0;
    
    if(depth == 0) return 1;
    
    n = generate_moves(movelist);
    for(i = 0; i < n; i += 1)
    {
        make_move(movelist[i]);
        result += perft(depth - 1);
        unmake_move(movelist[i]);
    }
    return result;
}

#define check_perft_on_position(depth, fen) \
setup_position(fen);\
printf("%d\n", perft(depth))




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

int evaluate()
{
    int evaluation = 0;
    
    int i;
    for(i = 0; i < 64; i += 1)
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



int history[32][120][120];
void clear_history()
{
    int i;
    for(i = 0; i < 32; i += 1)
    {
        int j;
        for(j = 0; j < 120; j += 1)
        {
            int k;
            for(k = 0; k < 120; k += 1)
            {
                history[i][j][k] = 0;
            }
        }
    }
}

int value_for_mvvlva(int figure)
{
    switch(get_value(figure))
    {
        case QUEEN : return 6;
        case ROOK  : return 5;
        case BISHOP: return 4;
        case KNIGHT: return 3;
        case PAWN  : return 2;
        default    : return 1;
    }
}

void sorting_captures(Move *movelist, int n)
{
    int sorting_values[n];
    int i;
    for(i = 0; i < n; i += 1)
    {
        int figure = value_for_mvvlva(board[movelist[i].from]);
        int broken = value_for_mvvlva(movelist[i].broken);
        sorting_values[i] = broken * 10 - figure;
    }
    for(i = 1; i < n; i += 1)
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
    int i;
    for(i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        if(i_move.broken)
        {
            int figure = value_for_mvvlva(board[i_move.to]);
            int broken = value_for_mvvlva(i_move.broken);
            sorting_values[i] = 100000*(broken * 10 - figure);
        }
        else
        {
            sorting_values[i] = history[board[i_move.from]][i_move.from][i_move.to];
        }
    }
    for(i = 1; i < n; i += 1)
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

int quiescence(int alpha, int beta)
{
    int static_evaluation = evaluate();
    if(static_evaluation >= beta)
        return beta;
    if(static_evaluation > alpha)
        alpha = static_evaluation;
    
    Move movelist[256];
    int n = generate_captures(movelist);
    sorting_captures(movelist, n);
    int i;
    for(i = 0; i < n; i += 1)
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

int alphabeta(int alpha, int beta, int depth)
{
    if(depth == 0) return quiescence(alpha, beta);
    
    Move movelist[256];
    int n = generate_moves(movelist);
    if(n == 0)
    {
        if(not_in_check(turn_to_move))
            return DRAW;
        return LOSING + ply - begin_ply;
    }
    sorting_moves(movelist, n);
    int i;
    for(i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        int score = -alphabeta(-beta, -alpha, depth - 1);
        unmake_move(i_move);
        
        if(score >= beta)
        {
            if(!i_move.broken)
            {
                history[board[i_move.from]][i_move.from][i_move.to] = depth * depth;
            }
            return beta;
        }
        if(score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

Move search(int depth)
{
    int alpha = -1000000, beta = 1000000;
    Move movelist[256];
    
    int n = generate_moves(movelist);
    sorting_moves(movelist, n);
    
    Move bestmove = {};
    int i;
    for(i = 0; i < n; i += 1)
    {
        Move i_move = movelist[i];
        make_move(i_move);
        int score = -alphabeta(-beta, -alpha, depth - 1);
        unmake_move(i_move);
        
        if(score > alpha)
        {
            bestmove = i_move;
            alpha = score;
        }
    }
    return bestmove;
}


//game with engine

Move parse_move(char *string)
{
    Move None = {0, 0, 0, 0};
    Move move = {};
    int i;
    for(i = 0; i < 4; i += 1)
        if(string[i] == '\0') return None;
    if(!((string[0] >= 'a' && string[0] <= 'h') &&
         (string[1] >= '1' && string[1] <= '8') &&
         (string[2] >= 'a' && string[2] <= 'h') &&
         (string[3] >= '1' && string[3] <= '8'))) return None;
    move.from = ('8' - string[1] + 2) * 10 + (string[0] - 'a' + 1);
    move.to   = ('8' - string[3] + 2) * 10 + (string[2] - 'a' + 1);
    move.broken = board[move.to];
    switch(string[4])
    {
        case '\0': break;
        case 'q': move.turn = create_figure(turn_to_move, QUEEN); break;
        case 'r': move.turn = create_figure(turn_to_move, ROOK); break;
        case 'b': move.turn = create_figure(turn_to_move, BISHOP); break;
        case 'n': move.turn = create_figure(turn_to_move, KNIGHT); break;
        default: return None;
    }
    return move;
}

int main()
{
    Move movelist[256];
    int n;
    int default_depth = 6;
    clear_history();
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
            int i;
            for(i = 0; i < n; i += 1)
            {
                Move i_move = movelist[i]; 
                if(move.from == i_move.from && move.to == i_move.to &&
                    move.turn == i_move.turn)
                {
                    make_move(move);
                    flag = 0;
                }
            }
        }
        
        print_position();
        Move bestmove = search(default_depth);
        if(bestmove.from == 0)
            break;
        make_move(bestmove);
    }
    return 0;
}
