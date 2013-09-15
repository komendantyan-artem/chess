import random
BORDER = 100
KING   = 1
QUEEN  = 2
ROOK   = 3
BISHOP = 4
KNIGHT = 5
PAWN   = 6
WHITE =  1
BLACK = -1
color = lambda x: None if x == 0 or x == BORDER else WHITE if x > 0 else BLACK
type_of = abs

moves_of_knight = ((2,1),(-2,1),(2,-1),(-2,-1),(1,2),(-1,2),(1,-2),(-1,-2))
directions_of_bishop = ((1,1),(-1,1),(1,-1),(-1,-1))
directions_of_rook = ((1,0),(-1,0),(0,1),(0,-1))
directions_of_queen = directions_of_rook + directions_of_bishop
moves_of_king = directions_of_queen
get_directions = (lambda x:directions_of_bishop if x == BISHOP else
                           directions_of_rook   if x == ROOK else
                           directions_of_queen)

captures_in_begin = lambda move: move.broken == 0

LOSING = -300000
DRAW = 0
'''PST from
http://chessprogramming.wikispaces.com/Simplified+evaluation+function
with some bad corrections
'''
#Material: PAWN = 1, BISHOP = KNIGHT = 3, ROOK = 5, QUEEN = 9
PST_EMPTY = [
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0],
    [0,0,0,0,0,0,0,0]]
PST_W_KING = [
    [ 20, 30, 10, 00, 00, 10, 30, 20],
    [ 20, 20, 00, 00, 00, 00, 20, 20],
    [-10,-20,-20,-20,-20,-20,-20,-10],
    [-30,-30,-40,-50,-50,-40,-30,-30],
    [-30,-30,-40,-50,-50,-40,-30,-30],
    [-30,-30,-40,-50,-50,-40,-30,-30],
    [-30,-30,-40,-50,-50,-40,-30,-30],
    [-30,-30,-40,-50,-50,-40,-30,-30]]
PST_W_QUEEN = [
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],
    [900,900,900,900,900,900,900,900],]
PST_W_ROOK = [
    [500,500,500,505,505,500,500,500],
    [495,500,500,500,500,500,500,495],
    [495,500,500,500,500,500,500,495],
    [495,500,500,500,500,500,500,495],
    [495,500,500,500,500,500,500,495],
    [495,500,500,500,500,500,500,495],
    [505,510,510,510,510,510,510,505],
    [500,500,500,500,500,500,500,500]]
PST_W_BISHOP = [
    [280,290,290,290,290,290,290,280],
    [290,305,300,300,300,300,305,290],
    [290,310,310,310,310,310,310,290],
    [290,300,310,310,310,310,300,290],
    [290,305,305,310,310,305,305,290],
    [290,300,305,310,310,305,300,290],
    [290,300,300,300,300,300,300,290],
    [280,290,290,290,290,290,290,280]]
PST_W_KNIGHT = [
    [250,260,270,270,270,270,260,250],
    [260,280,300,305,305,300,280,260],
    [270,305,310,315,315,310,305,270],
    [270,300,315,320,320,315,300,270],
    [270,305,315,320,320,315,305,270],
    [270,300,310,315,315,310,300,270],
    [260,280,300,300,300,300,280,260],
    [250,260,270,270,270,270,260,250]]
PST_W_PAWN = [
    [000,000,000,000,000,000,000,000],
    [105,110,110, 80, 80,110,110,105],
    [105, 95, 90,100,100, 90, 95,105],
    [100,100,100,120,120,100,100,100],
    [105,105,110,125,125,110,105,105],
    [110,110,120,130,130,120,110,110],
    [150,150,150,150,150,150,150,150],
    [000,000,000,000,000,000,000,000]]
PST_B_KING   = [[-j for j in i] for i in reversed(PST_W_KING  )]
PST_B_QUEEN  = [[-j for j in i] for i in reversed(PST_W_QUEEN )]
PST_B_ROOK   = [[-j for j in i] for i in reversed(PST_W_ROOK  )]
PST_B_BISHOP = [[-j for j in i] for i in reversed(PST_W_BISHOP)]
PST_B_KNIGHT = [[-j for j in i] for i in reversed(PST_W_KNIGHT)]
PST_B_PAWN   = [[-j for j in i] for i in reversed(PST_W_PAWN  )]
PST = {
             0     : PST_EMPTY,
             KING  : PST_W_KING,
             QUEEN : PST_W_QUEEN,
             ROOK  : PST_W_ROOK,
             BISHOP: PST_W_BISHOP,
             KNIGHT: PST_W_KNIGHT,
             PAWN  : PST_W_PAWN,
            -KING  : PST_B_KING,
            -QUEEN : PST_B_QUEEN,
            -ROOK  : PST_B_ROOK,
            -BISHOP: PST_B_BISHOP,
            -KNIGHT: PST_B_KNIGHT,
            -PAWN  : PST_B_PAWN,            
      }



class Move:
    def __init__(self, out=(0,0), to=(0,0), broken=0, en_passant=0, turn=0):
        self.out = out
        self.to = to
        self.broken = broken
        self.en_passant = en_passant
        self.turn = turn
        self.copies_of_flags = None


class Position:
    def __init__(self):
        self.position = [[0 for i in xrange(12)] for j in xrange(12)]
        for i in xrange(12):
            for j in xrange(2):
                self.position[i][j] = BORDER
                self.position[i][11-j] = BORDER
                self.position[j][i] = BORDER
                self.position[11-j][i] = BORDER
        self.turn_to_move = 0
        self.castling = [0,0,0,0]
        self.en_passant = 0

    def __str__(self):
        figure_to_str = dict(zip(range(1, 7) + range(-1,-7,-1) + [0],
                             "KQRBNPkqrbnp."))
        string = ""
        for i in reversed(self.position[2:10]):
            string += '\n'
            for j in i[2:10]:
                string += figure_to_str[j]
        return string

    def setup(self, fen=None):
        if fen == None:
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        tmp = fen.split()
        arrangement_of_figures = tmp[0]
        turn_to_move           = tmp[1]
        possible_castling      = tmp[2]
        en_passant             = tmp[3]
        number_of_insignificant_plies = int(tmp[4])
        number_of_moves               = int(tmp[5])
        
        horizontal, vertical = 9, 2
        str_to_figure = dict(zip("KQRBNPkqrbnp",
                             range(1, 7) + range(-1,-7,-1)))
        for i in arrangement_of_figures:
            if i == '/':
                horizontal -= 1
                vertical = 2
            elif i in str_to_figure:
                self.position[horizontal][vertical] = str_to_figure[i]
                vertical += 1
            else:
                number = int(i)
                for j in xrange(number):
                    self.position[horizontal][vertical+j] = 0
                vertical += number
        
        self.turn_to_move = 1 if turn_to_move == 'w' else -1
        
        self.castling[0] = 1 if 'K' in possible_castling else 0
        self.castling[1] = 1 if 'Q' in possible_castling else 0
        self.castling[2] = 1 if 'k' in possible_castling else 0
        self.castling[3] = 1 if 'q' in possible_castling else 0
        
        if en_passant != '-':
            self.en_passant = 2 + "abcdefgh".index(en_passant[0])
        else:
            self.en_passant = 0
    
    def make_move(self, move):
        move.copies_of_flags = (self.castling[:], self.en_passant)
        h1, v1 = move.out
        h2, v2 = move.to
        figure = self.position[h1][v1]
        self.position[h2][v2] = figure
        self.position[h1][v1] = 0
        if move.turn != 0:
            self.position[h2][v2] = move.turn
        if move.en_passant:
            self.position[h1][v2] = 0
        if type_of(figure) == PAWN and abs(h1 - h2) == 2:
            self.en_passant = v1
        else:
            self.en_passant = 0
        if type_of(figure) == KING:
            add_to_index = 0 if self.turn_to_move == 1 else 2
            self.castling[add_to_index] = 0
            self.castling[1 + add_to_index] = 0
            if v1 - v2 == 2:
                self.position[h1][5] = self.position[h1][2]
                self.position[h1][2] = 0
            elif v2 - v1 == 2:
                self.position[h1][7] = self.position[h1][9]
                self.position[h1][9] = 0
        for color_of_rooks in (1, -1):
            add_to_index = 0 if color_of_rooks == 1 else 2
            horizontal_1 = 2 if color_of_rooks == 1 else 9
            place_of_rook_1 = self.position[horizontal_1][9]
            place_of_rook_2 = self.position[horizontal_1][2]
            if(color(place_of_rook_1) != color_of_rooks or
              type_of(place_of_rook_1) != ROOK):
                self.castling[add_to_index] = 0
            if(color(place_of_rook_2) != color_of_rooks or
              type_of(place_of_rook_2) != ROOK):
                self.castling[1 + add_to_index] = 0
        self.turn_to_move = -self.turn_to_move

    def unmake_move(self, move):
        self.castling, self.en_passant = move.copies_of_flags
        self.turn_to_move = -self.turn_to_move
        h1, v1 = move.out
        h2, v2 = move.to
        self.position[h1][v1] = self.position[h2][v2]
        self.position[h2][v2] = move.broken
        figure = self.position[h1][v1]
        if move.turn != 0:
            self.position[h1][v1] = PAWN * self.turn_to_move
        elif move.en_passant:
            self.position[h2][v2] = 0
            self.position[h1][v2] = move.broken
        elif type_of(figure) == KING and abs(v1 - v2) == 2:
            if v2 < v1:
                self.position[h1][2] = self.position[h1][5]
                self.position[h1][5] = 0
            else:
                self.position[h1][9] = self.position[h1][7]
                self.position[h1][7] = 0

    def in_check(self, color_of_king):
        place_of_king = None
        for i in xrange(2, 10):
            for j in xrange(2, 10):
                tmp = self.position[i][j]
                if(type_of(tmp) == KING and color(tmp) == color_of_king):
                      place_of_king = (i, j)
                      break
            if place_of_king != None:
                break
        x, y = place_of_king
        direction_of_pawns = color_of_king
        for k in (1, -1):
            tmp = self.position[x + direction_of_pawns][y + k]
            if type_of(tmp) == PAWN and color(tmp) == -color_of_king:
                return True
        for k, l in moves_of_knight:
            tmp = self.position[x + k][y + l]
            if type_of(tmp) == KNIGHT and color(tmp) == -color_of_king:
                return True
        for k, l in moves_of_king:
            tmp = self.position[x + k][y + l]
            if type_of(tmp) == KING and color(tmp) == -color_of_king:
                return True
        for k, l in directions_of_rook:
            for n in xrange(1, 10):
                tmp = self.position[x + k*n][y + l*n]
                if(color(tmp) == -color_of_king and
                  type_of(tmp) in (QUEEN, ROOK)):
                    return True
                if tmp != 0:
                    break
        for k, l in directions_of_bishop:
            for n in xrange(1, 10):
                tmp = self.position[x + k*n][y + l*n]
                if(color(tmp) == -color_of_king and
                  type_of(tmp) in (QUEEN, BISHOP)):
                    return True
                if tmp != 0:
                    break
        return False

    def generate_moves(self):
        def castling_is_possible(horizontal_of_king, which_castling):
            add_to_index = 0 if self.turn_to_move == 1 else 2
            if not self.castling[which_castling + add_to_index]:
                return False
            if self.in_check(self.turn_to_move):
                return False
            x = horizontal_of_king
            if which_castling == 0:
                for i in xrange(7, 9):
                    if self.position[x][i] != 0:
                        return False
                move = Move(out=(x, 6),to=(x, 7))
            else:
                for i in xrange(3, 6):
                    if self.position[x][i] != 0:
                        return False
                move = Move(out=(x, 6),to=(x, 5))
            self.make_move(move)
            if self.in_check(-self.turn_to_move):
                self.unmake_move(move)
                return False
            self.unmake_move(move)
            return True
                
        possible_moves = []
        direction_of_pawns = self.turn_to_move
        horizontal_2 = 3 if direction_of_pawns == 1 else 8
        for i in xrange(2, 10):
            for j in xrange(2, 10):
                if color(self.position[i][j]) != self.turn_to_move:
                    continue
                type_of_figure = type_of(self.position[i][j])
                if type_of_figure == KNIGHT:
                    for k, l in moves_of_knight:
                        tmp = self.position[i + k][j + l]
                        if tmp == 0 or color(tmp) == -self.turn_to_move:
                            possible_moves.append(Move(out=(i, j),
                                to=(i + k, j + l), broken=tmp))
                elif type_of_figure in (QUEEN, ROOK, BISHOP):
                    directions = get_directions(type_of_figure)
                    for k, l in directions:
                        for n in xrange(1,10):
                            tmp = self.position[i + k*n][j + l*n]
                            if tmp != 0 and color(tmp) != -self.turn_to_move:
                                break
                            possible_moves.append(Move(out=(i, j),
                                to=(i + k*n, j + l*n), broken=tmp))
                            if color(tmp) == -self.turn_to_move:
                                break
                elif type_of_figure == PAWN:
                    if self.position[i + direction_of_pawns][j] == 0:
                        if i + direction_of_pawns in (2, 9):
                            for figure in (QUEEN, ROOK, BISHOP, KNIGHT):
                                possible_moves.append(Move(out=(i, j),
                                    to=(i + direction_of_pawns, j),
                                    turn = figure*self.turn_to_move))
                        else:
                            possible_moves.append(Move(out=(i, j),
                                to=(i + direction_of_pawns, j)))
                        if(i == horizontal_2 and
                          self.position[i + 2*direction_of_pawns][j] == 0):
                            possible_moves.append(Move(out=(i, j),
                                to=(i + 2*direction_of_pawns, j)))
                    for k in (-1, 1):
                        tmp = self.position[i + direction_of_pawns][j + k]
                        if color(tmp) == -self.turn_to_move:
                            if i + direction_of_pawns in (2, 9):
                                for figure in (QUEEN, ROOK, BISHOP, KNIGHT):
                                    possible_moves.append(Move(out=(i, j),
                                        to=(i + direction_of_pawns, j + k),
                                        broken=tmp,
                                        turn = figure*self.turn_to_move))
                            else:
                                possible_moves.append(Move(out=(i, j),
                                    to=(i + direction_of_pawns, j + k),
                                    broken=tmp))
                        elif(self.en_passant == j + k and 
                          i == horizontal_2 + 3*direction_of_pawns):
                            possible_moves.append(Move(out=(i, j),
                                to=(i + direction_of_pawns, j + k),
                                broken=(PAWN * -self.turn_to_move),
                                en_passant=1))
                elif type_of_figure == KING:
                    for k, l in moves_of_king:
                        tmp = self.position[i + k][j + l]
                        if tmp == 0 or color(tmp) == -self.turn_to_move:
                            possible_moves.append(Move(out=(i, j),
                                to=(i + k, j + l), broken=tmp))
                    if castling_is_possible(i, 0):
                        possible_moves.append(Move(out=(i, 6), to=(i, 8)))
                    if castling_is_possible(i, 1):
                        possible_moves.append(Move(out=(i, 6), to=(i, 4)))

        possible_moves_2 = []
        for i in possible_moves:
            self.make_move(i)
            if not self.in_check(-self.turn_to_move):
                possible_moves_2.append(i)
            self.unmake_move(i)
        return possible_moves_2
    
    def perft(self, depth):
        if depth == 0:
            return 1
        possible_moves = self.generate_moves()
        result = 0
        for i in possible_moves:
            self.make_move(i)
            result += self.perft(depth - 1)
            self.unmake_move(i)
        return result
    
    def evaluate(self):
        #PST: declaration in begin.
        evaluation = 0
        for i in xrange(2, 10):
            for j in xrange(2, 10):
                tmp = self.position[i][j]
                evaluation += PST[tmp][i-2][j-2]
        return evaluation
    
    def quiescence(self, alpha, beta):
        possible_moves = self.generate_moves()
        if len(possible_moves) == 0:
            if self.in_check(self.turn_to_move):
                return LOSING
            return DRAW
        evaluation = self.evaluate() * self.turn_to_move
        if evaluation >= beta:
            return evaluation
        if evaluation > alpha:
            alpha = evaluation
        possible_moves.sort(key=captures_in_begin)
        for move in possible_moves:
            if move.broken == 0:
                break
            self.make_move(move)
            score = -self.quiescence(-beta, -alpha)
            self.unmake_move(move)
            if score >= beta:
                return beta
            if score > alpha:
                alpha = score
        return alpha
    
    def alphabeta(self, alpha, beta, depth):
        if depth == 0:
            return self.quiescence(alpha, beta)
        possible_moves = self.generate_moves()
        if len(possible_moves) == 0:
            if self.in_check(self.turn_to_move):
                return LOSING
            return DRAW
        possible_moves.sort(key=captures_in_begin)
        for move in possible_moves:
            self.make_move(move)
            score = -self.alphabeta(-beta, -alpha, depth-1)
            self.unmake_move(move)
            if score >= beta:
                return beta
            if score > alpha:
                alpha = score
        return alpha
    
    def search(self, depth=2):
        #TODO: sort moves
        alpha, beta = -1000000, 1000000
        bestmove = None
        possible_moves = self.generate_moves()
        possible_moves.sort(key=captures_in_begin)
        for move in possible_moves:
            self.make_move(move)
            score = -self.alphabeta(-beta, -alpha, depth-1)
            self.unmake_move(move)
            if score > alpha:
                bestmove = move
                alpha = score
        return bestmove
                    
                    



tmp = open("test_positions1.py")
test_positions = eval(tmp.read())
tmp.close()

test1 = '''
log = open("log.txt", 'w')
number_of_incorrect = 0
number = 0
p = Position()
depth = 2
for i in test_positions:
    number += 1
    p.setup(i[0])
    result = p.perft(depth)
    if i[depth] != result:
        number_of_incorrect += 1
        log.write("INCORRECT!!!")
    log.write("{0})".format(number))
    log.write(str(p))
    log.write("\nCORRECT: {0} OUR: {1}".format(i[depth], result))
    log.write("\n")
log.write("INCORRECT: " + str(number_of_incorrect))
log.close()
#'''

test2 = '''
import time
p = Position()
p.setup()
begin = time.time()
print p.perft(4)
print time.time() - begin
#'''

test3 = '''
log = open("log.txt", 'w')
p = Position()
i = test_positions[1]
p.setup(i[0])
log.write(str(p))
log.write("\n")
for move in p.generate_moves():
    p.make_move(move)
    log.write(str(p))
    log.write("\nOUR: {0}\n".format(p.perft(1)))
    log.write(str([(k.out, k.to) for k in p.generate_moves()]))
    p.unmake_move(move)
#'''

#play_with_engine = '''
def parse_move(string):
    turn = {'q': QUEEN, 'r': ROOK, 'b': BISHOP, 'n': KNIGHT}
    vertical =   dict(zip("abcdefgh", range(2, 10)))
    horizontal = dict(zip("12345678", range(2, 10)))
    result = [0 for i in xrange(3)]
    if len(string) == 5:
        if string[4] not in turn:
            return None
        result[2]  = turn[string[4]]
    elif len(string) != 4:
        return None
    if not(string[0] in vertical   and string[2] in vertical and
           string[1] in horizontal and string[3] in horizontal):
        return None
    result[0] = (horizontal[string[1]], vertical[string[0]])
    result[1] = (horizontal[string[3]], vertical[string[2]])
    return result
    

default_depth = 2
p = Position()
p.setup()
while True:
    print p
    possible_moves = p.generate_moves()
    while True:
        move = parse_move(raw_input())
        if move == None:
            continue
        flag = 0
        for i in possible_moves:
            if move[0] == i.out and move[1] == i.to:
                i.turn = move[2]
                p.make_move(i)
                flag = 1
                break
        if flag:
            break
    
    print p
    bestmove = p.search(depth=default_depth)
    if bestmove == None:
        break
    p.make_move(bestmove)
#'''
