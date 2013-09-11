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
} flags[1000], *ply;

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
}

void setup_position(char* fen)
{
    int i, current_cell;
    ply = flags;
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

void setup_start_position()
{
    setup_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
    if(number_of_atackers == 0)
        for(i = 0; i < 120; i += 1) defend_against_check[i] = 1;
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
            
            if(!i_move_is_possible || board[tmp] != EMPTY)
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
                Move tmp_move = {.from = tmp, .to = ply->en_passant};
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
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
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
                    if(rentgen[current_cell][0] &&
                       inc != rentgen[current_cell][1] &&
                       -inc != rentgen[current_cell][1])
                            continue;
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
                if(!rentgen[current_cell][0])
                {
                    for(i = 0; i < 8; i += 1)
                    {
                        int tmp = current_cell + moves_of_knight[i];
                        if(defend_against_check[tmp] &&
                            (board[tmp] == EMPTY ||
                             get_color(board[tmp]) == not_turn_to_move))
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
                    if(defend_against_check[tmp])
                    {
                        if(current_cell/10 == horizontal7)
                        {
                            for(i = 0; i < 4; i += 1)
                            {
                                Move tmp_move =
                                    {.from = current_cell, .to = tmp,
                                    .turn = create_figure(turn_to_move, 
                                    turn_figures[i])};
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
                    if(board[tmp] == EMPTY && current_cell/10 == horizontal2 &&
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
                    if(!(rentgen[current_cell][0] &&
                         abs(rentgen[current_cell][1]) !=
                             abs(captures_of_pawns[i])) &&
                         defend_against_check[tmp] &&
                         get_color(board[tmp]) == not_turn_to_move)
                    {
                        if(current_cell/10 == horizontal7)
                        {
                            for(i = 0; i < 4; i += 1)
                            {
                                Move tmp_move =
                                    {.from = current_cell, .to = tmp,
                                    .broken = board[tmp],
                                    .turn = create_figure(turn_to_move, 
                                    turn_figures[i])};
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

#define check_perft_on_position(fen) \
setup_position(fen);\
printf("%d\n", perft(default_depth))
int main()
{
    //tests
    
    //setup_start_position();
    /*int i;
    Move movelist[300];
    int n = generate_moves(movelist);
    for(i = 0; i < n; i += 1) printf("%d %d ||", movelist[i].from, movelist[i].to); 
    printf("\n");*/
    
    int default_depth = 3;
    check_perft_on_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    //check_perft_on_position("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
    return 0;
}
