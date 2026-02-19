#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>

const float GROUND_Y = -0.7f;
const float GRAVITY = -0.0018f;
const float JUMP_POWER = 0.045f;
const float WORLD_SPEED = 0.012f;
const float RABBIT_X = -0.7f;
const float FOX_SPEED = WORLD_SPEED * 0.8f;  // Slower fox

enum GameState { RUNNING, GAME_OVER };
GameState gameState = RUNNING;

float rabbitY = GROUND_Y;
float velocityY = 0.0f;
bool onGround = true;
float runPhase = 0.0f;

float foxX = -1.0f;
float foxY = GROUND_Y + 0.05f;
bool foxChasing = false;
int foxTimer = 0;
const int FOX_DURATION = 300; // 300 frames * 16ms ≈ 4.8 seconds (close to 5 seconds)
bool hasHitObstacleDuringChase = false; // Track if we've already hit an obstacle during current chase

int score = 0;

struct GameObject {
    float x, y, size;
    bool active;
};
std::vector<GameObject> carrots;
std::vector<GameObject> obstacles;
std::vector<GameObject> pits;

struct Tree {
    float x, y, scale;
};
std::vector<Tree> trees;

// -------------------- DRAWING UTILITIES --------------------
void drawRectangle(float cx, float cy, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(cx - width/2, cy - height/2);
    glVertex2f(cx + width/2, cy - height/2);
    glVertex2f(cx + width/2, cy + height/2);
    glVertex2f(cx - width/2, cy + height/2);
    glEnd();
}

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 40; i++) {
        float a = 2.0f * 3.1416f * i / 40;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void drawOval(float cx, float cy, float rx, float ry) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 50; i++) {
        float a = 2.0f * 3.1416f * i / 50;
        glVertex2f(cx + cosf(a) * rx, cy + sinf(a) * ry);
    }
    glEnd();
}

// -------------------- CARROT, PIT, TREE --------------------
void drawCarrot(float x, float y) {
    glColor3f(1.0f, 0.5f, 0.0f);
    drawOval(x, y - 0.02f, 0.02f, 0.05f);
    glColor3f(0.0f, 0.8f, 0.0f);
    drawOval(x, y + 0.025f, 0.015f, 0.03f);
}

void drawPit(float x, float width) {
    glColor3f(0.15f, 0.5f, 0.1f);
    drawOval(x, GROUND_Y, width, 0.02f);
}

void drawTree(float x, float y, float scale = 1.0f) {
    glColor3f(0.55f, 0.27f, 0.07f);
    drawOval(x, y, 0.02f*scale, 0.05f*scale);

    glColor3f(0.0f, 0.6f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 0.05f*scale, y + 0.03f*scale);
    glVertex2f(x + 0.05f*scale, y + 0.03f*scale);
    glVertex2f(x, y + 0.12f*scale);

    glVertex2f(x - 0.04f*scale, y + 0.07f*scale);
    glVertex2f(x + 0.04f*scale, y + 0.07f*scale);
    glVertex2f(x, y + 0.16f*scale);

    glVertex2f(x - 0.03f*scale, y + 0.11f*scale);
    glVertex2f(x + 0.03f*scale, y + 0.11f*scale);
    glVertex2f(x, y + 0.2f*scale);
    glEnd();
}

// -------------------- COLLISION --------------------
bool collide(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx*dx + dy*dy < (r1+r2)*(r1+r2);
}

// -------------------- RABBIT --------------------
void drawRabbit(float x, float y) {
    float legSwing = onGround ? sinf(runPhase) * 0.025f : 0.0f;
    float earBounce = -velocityY * 0.4f;
    float tilt = sinf(runPhase*1.5f)*0.2f;

    glColor3f(0.95f,0.95f,0.95f); drawOval(x,y,0.085f,0.055f);
    glColor3f(0.9f,0.9f,0.9f);
    drawOval(x-0.045f, y-0.04f+legSwing, 0.045f,0.03f);
    drawOval(x+0.025f, y-0.045f-legSwing, 0.022f,0.028f);
    glColor3f(0.97f,0.97f,0.97f); drawOval(x+0.095f, y+0.03f,0.04f,0.035f);
    glColor3f(0.95f,0.95f,0.95f);
    drawOval(x+0.085f+tilt*0.02f, y+0.08f+earBounce,0.015f,0.045f);
    drawOval(x+0.115f+tilt*-0.02f, y+0.08f+earBounce,0.015f,0.045f);
    glColor3f(1.0f,0.85f,0.85f);
    drawOval(x+0.085f+tilt*0.02f, y+0.08f+earBounce,0.007f,0.03f);
    drawOval(x+0.115f+tilt*-0.02f, y+0.08f+earBounce,0.007f,0.03f);
    glColor3f(0,0,0); drawCircle(x+0.105f, y+0.04f,0.005f);
    glColor3f(0.9f,0.6f,0.6f); drawCircle(x+0.13f, y+0.025f,0.004f);
    glColor3f(1,1,1); drawCircle(x-0.10f, y+0.02f,0.02f);
}

// -------------------- FOX --------------------
void drawFox() {

    
    // Main body
    glColor3f(1.0f, 0.4f, 0.2f); // Orange-red color
    drawOval(foxX, foxY, 0.09f, 0.05f);
    
    // Head - slightly larger to merge with ears
    glColor3f(1.0f, 0.5f, 0.2f);
    drawOval(foxX + 0.08f, foxY + 0.03f, 0.06f, 0.05f); // Slightly larger and higher
    
    // Ears (triangular - positioned to merge with head)
    glColor3f(1.0f, 0.4f, 0.1f);
    
    // Left ear - positioned to overlap with head
    glBegin(GL_TRIANGLES);
    glVertex2f(foxX + 0.03f, foxY + 0.05f);  // Base left (inside head)
    glVertex2f(foxX + 0.07f, foxY + 0.05f);  // Base right (inside head)
    glVertex2f(foxX + 0.05f, foxY + 0.12f);  // Top point
    glEnd();
    
    // Right ear - positioned to overlap with head
    glBegin(GL_TRIANGLES);
    glVertex2f(foxX + 0.11f, foxY + 0.05f);  // Base left (inside head)
    glVertex2f(foxX + 0.15f, foxY + 0.05f);  // Base right (inside head)
    glVertex2f(foxX + 0.13f, foxY + 0.12f);  // Top point
    glEnd();
    
    // Ear inner parts (lighter/pinker)
    glColor3f(1.0f, 0.7f, 0.5f);
    
    // Left inner ear
    glBegin(GL_TRIANGLES);
    glVertex2f(foxX + 0.04f, foxY + 0.06f);
    glVertex2f(foxX + 0.06f, foxY + 0.06f);
    glVertex2f(foxX + 0.05f, foxY + 0.10f);
    glEnd();
    
    // Right inner ear
    glBegin(GL_TRIANGLES);
    glVertex2f(foxX + 0.12f, foxY + 0.06f);
    glVertex2f(foxX + 0.14f, foxY + 0.06f);
    glVertex2f(foxX + 0.13f, foxY + 0.10f);
    glEnd();
    
    // Add a small fur patch to cover the ear-head junction
    glColor3f(1.0f, 0.5f, 0.2f); // Same as head color
    drawOval(foxX + 0.05f, foxY + 0.04f, 0.02f, 0.01f); // Left ear base cover
    drawOval(foxX + 0.13f, foxY + 0.04f, 0.02f, 0.01f); // Right ear base cover
    
    // Eyes
    glColor3f(1, 1, 1);
    drawCircle(foxX + 0.07f, foxY + 0.04f, 0.008f);
    drawCircle(foxX + 0.11f, foxY + 0.04f, 0.008f);
    
    glColor3f(0, 0, 0);
    drawCircle(foxX + 0.07f, foxY + 0.04f, 0.004f);
    drawCircle(foxX + 0.11f, foxY + 0.04f, 0.004f);
    
    // Eye highlights
    glColor3f(1, 1, 1);
    drawCircle(foxX + 0.068f, foxY + 0.042f, 0.0015f);
    drawCircle(foxX + 0.108f, foxY + 0.042f, 0.0015f);
    
    // Nose (black triangle - inverted/pointing down)
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(foxX + 0.13f, foxY + 0.03f);  // Top
    glVertex2f(foxX + 0.14f, foxY + 0.025f); // Bottom right
    glVertex2f(foxX + 0.135f, foxY + 0.02f); // Bottom point
    glEnd();
    
    // Whiskers
    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    // Left whiskers
    glVertex2f(foxX + 0.12f, foxY + 0.03f);
    glVertex2f(foxX + 0.10f, foxY + 0.025f);
    glVertex2f(foxX + 0.12f, foxY + 0.03f);
    glVertex2f(foxX + 0.10f, foxY + 0.035f);
    // Right whiskers
    glVertex2f(foxX + 0.15f, foxY + 0.03f);
    glVertex2f(foxX + 0.17f, foxY + 0.025f);
    glVertex2f(foxX + 0.15f, foxY + 0.03f);
    glVertex2f(foxX + 0.17f, foxY + 0.035f);
    glEnd();
    
    // Legs
    glColor3f(1.0f, 0.4f, 0.2f);
    // Front leg
    drawRectangle(foxX + 0.03f, GROUND_Y + 0.02f, 0.02f, 0.04f);
    // Back leg
    drawRectangle(foxX - 0.05f, GROUND_Y + 0.02f, 0.02f, 0.04f);
    
    // Tail (bushy)
    glColor3f(1.0f, 0.5f, 0.2f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(foxX - 0.12f, foxY - 0.01f);
    for (int i = 0; i <= 20; i++) {
        float a = 3.1416f * i / 20; // Half circle
        glVertex2f(foxX - 0.12f + cosf(a) * 0.06f, foxY - 0.01f + sinf(a) * 0.03f);
    }
    glEnd();
    
    // Tail tip (white)
    glColor3f(1, 1, 1);
    drawCircle(foxX - 0.17f, foxY - 0.01f, 0.015f);
    
    // Reset line width
    glLineWidth(1.0f);
}

// -------------------- SPAWN --------------------
void spawnObjects() {
    carrots.clear(); obstacles.clear(); pits.clear(); trees.clear();
    float xPos = 1.5f;
    for(int i=0;i<5;i++){
        int groupSize = 1 + rand()%4;
        for(int j=0;j<groupSize;j++){
            float spacing = 0.04f + (rand()%3)/100.0f;
            carrots.push_back({xPos, GROUND_Y+0.05f, 0.05f,true});
            xPos += spacing;
        }
        xPos += 0.5f + (rand()%3)/10.0f;
    }
    for(int i=0;i<5;i++){
        obstacles.push_back({2.5f+i*1.8f, GROUND_Y+0.05f,0.07f,true});
        pits.push_back({3.0f+i*2.0f, GROUND_Y,0.08f,true});
        trees.push_back({-1.0f+i*0.8f, GROUND_Y, 0.8f+float(i%2)*0.3f});
    }
    foxX = -0.9f; 
    foxChasing = false; 
    foxTimer = 0; 
    hasHitObstacleDuringChase = false;
    score = 0;
}

// -------------------- UPDATE LOOP --------------------
void update(int) {
    if(gameState==GAME_OVER) {
        glutPostRedisplay();
        glutTimerFunc(16,update,0);
        return;
    }

    // Physics
    velocityY += GRAVITY;
    rabbitY += velocityY;
    onGround = (rabbitY <= GROUND_Y);
    if(onGround){ rabbitY=GROUND_Y; velocityY=0; runPhase+=0.25f; }

    // Update carrots
    for(auto &c: carrots){
        c.x -= WORLD_SPEED;
        if(c.x < -1.2f){ 
            c.x = 2.0f + (rand()%100)/50.0f; 
            c.active = true; 
        }
        if(c.active && collide(RABBIT_X,rabbitY,0.06f,c.x,c.y,c.size)){ 
            c.active = false; 
            score += 10; 
        }
    }

    // Update obstacles - CRITICAL FIX HERE
    for(auto &o: obstacles){
        o.x -= WORLD_SPEED;

        if(o.x < -1.2f){
            o.x = 2.5f + (rand()%100)/50.0f;
            o.active = true;
        }

        // Check collision with active obstacles
        if(o.active && collide(RABBIT_X, rabbitY, 0.06f, o.x, o.y, o.size)){
            o.active = false; // Obstacle disappears after hit
            
            if(foxChasing) {
                // If we hit an obstacle while fox is chasing, check if this is the first hit during this chase
                if(hasHitObstacleDuringChase) {
                    // Second hit during same chase - GAME OVER
                    gameState = GAME_OVER;
                } else {
                    // First hit during this chase - mark it
                    hasHitObstacleDuringChase = true;
                }
            } else {
                // Not chasing yet - start the chase
                foxChasing = true;
                foxTimer = FOX_DURATION;
                hasHitObstacleDuringChase = true; // This is the first hit of the chase
                foxX = -0.9f; // Start fox closer to rabbit
            }
        }
    }

    // Update pits
    for(auto &p:pits){
        p.x -= WORLD_SPEED;
        if(p.x < -1.2f){ 
            p.x = 3.0f + (rand()%100)/50.0f; 
            p.active = true; 
        }
        if(p.active && RABBIT_X > p.x-0.04f && RABBIT_X < p.x+0.04f && onGround) {
            gameState = GAME_OVER;
        }
    }

    // Update trees (background)
    for(auto &t:trees){ 
        t.x -= WORLD_SPEED * 0.3f; 
        if(t.x < -1.2f) t.x = 1.2f; 
    }

    // --- FOX LOGIC ---
    if(foxChasing) {
        // Fox moves towards rabbit
        if(foxX < RABBIT_X -0.1f) {
            foxX += FOX_SPEED;
        }
        
        // Decrease timer
        foxTimer--;
        
        // Check if timer expired (chase duration over)
        if(foxTimer <= 0) {
            foxChasing = false;
            hasHitObstacleDuringChase = false; // Reset for next chase
            foxX = -0.9f; // Return to initial position
        }
        
        // Check if fox caught rabbit
        if(foxX >= RABBIT_X - 0.05f) {
            gameState = GAME_OVER;
        }
    } else {
        // Fox returns to right side when not chasing
        // Fox returns to left side when not chasing
if(foxX < -0.9f) {
    foxX += WORLD_SPEED * 0.2f; // Move back to left slowly
}
else if(foxX > -0.9f) {
    foxX = -0.9f;
}
        // Reset the hit flag when not chasing
        hasHitObstacleDuringChase = false;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// -------------------- DISPLAY --------------------
void display(){
    glClear(GL_COLOR_BUFFER_BIT);

    // Sky
    glColor3f(0.5f,0.8f,1.0f);
    glBegin(GL_QUADS); glVertex2f(-1,1); glVertex2f(1,1); glVertex2f(1,GROUND_Y); glVertex2f(-1,GROUND_Y); glEnd();
    // Ground
    glColor3f(0.2f,0.7f,0.2f);
    glBegin(GL_QUADS); glVertex2f(-1,GROUND_Y); glVertex2f(1,GROUND_Y); glVertex2f(1,-1); glVertex2f(-1,-1); glEnd();

    for(auto &p:pits) if(p.active) drawPit(p.x,p.size);
    for(auto &t:trees) drawTree(t.x,t.y,t.scale);
    for(auto &c:carrots) if(c.active) drawCarrot(c.x,c.y);

    glColor3f(0.5f,0.3f,0.1f);
    for(auto &o:obstacles) if(o.active) drawRectangle(o.x,o.y,o.size*2,o.size*0.5f);

    if(foxChasing) drawFox();
    drawRabbit(RABBIT_X,rabbitY);

    std::stringstream ss; ss << "Score: " << score;
    glColor3f(1,1,1); drawText(-0.95f,0.8f,ss.str());

    // Display chase status
    if(foxChasing) {
        std::stringstream timer; 
        timer << "CHASE! " << (foxTimer / 60) + 1 << "s remaining";
        glColor3f(1,0.2f,0); 
        drawText(-0.95f,0.7f,timer.str());
        
        // // Show warning if already hit once during this chase
        // if(hasHitObstacleDuringChase) {
        //     glColor3f(1,1,0); 
        //     drawText(-0.95f,0.6f,"WARNING: Next hit = GAME OVER!");
        // }
    }

    if(gameState == GAME_OVER){ 
        glColor3f(1,0,0); 
        drawText(-0.35f,0.0f,"GAME OVER - PRESS R TO RESTART"); 
    }

    glutSwapBuffers();
}

// -------------------- INPUT --------------------
void keyboard(unsigned char key, int, int){
    if(key == 32 && onGround){ // Space bar
        velocityY = JUMP_POWER; 
        onGround = false; 
    }
    if(gameState == GAME_OVER && (key == 'r' || key == 'R')){
        gameState = RUNNING; 
        rabbitY = GROUND_Y; 
        velocityY = 0; 
        runPhase = 0; 
        spawnObjects();
    }
}

// -------------------- INIT --------------------
void init(){
    glClearColor(0.5f, 0.8f, 1.0f, 1);
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    srand(time(NULL)); // Seed random number generator
    spawnObjects();
}

// -------------------- MAIN --------------------
int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(900, 600);
    glutCreateWindow("Run Rabbit - Fox Chase Game");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}