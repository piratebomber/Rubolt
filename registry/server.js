const express = require('express');
const multer = require('multer');
const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
const semver = require('semver');
const fs = require('fs').promises;
const path = require('path');
const crypto = require('crypto');
const tar = require('tar');
const { Pool } = require('pg');

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'rubolt-registry-secret';
const PACKAGES_DIR = process.env.PACKAGES_DIR || './packages';
const DATABASE_URL = process.env.DATABASE_URL || 'postgresql://localhost/rubolt_registry';

// Database connection
const pool = new Pool({
    connectionString: DATABASE_URL,
});

// Middleware
app.use(express.json({ limit: '50mb' }));
app.use(express.urlencoded({ extended: true }));

// File upload configuration
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, PACKAGES_DIR);
    },
    filename: (req, file, cb) => {
        const { name, version } = req.body;
        cb(null, `${name}-${version}.tgz`);
    }
});

const upload = multer({ 
    storage,
    limits: { fileSize: 100 * 1024 * 1024 }, // 100MB limit
    fileFilter: (req, file, cb) => {
        if (file.mimetype === 'application/gzip' || file.originalname.endsWith('.tgz')) {
            cb(null, true);
        } else {
            cb(new Error('Only .tgz files are allowed'));
        }
    }
});

// Authentication middleware
const authenticateToken = async (req, res, next) => {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        return res.status(401).json({ error: 'Access token required' });
    }

    try {
        const decoded = jwt.verify(token, JWT_SECRET);
        const user = await getUserById(decoded.userId);
        
        if (!user) {
            return res.status(403).json({ error: 'Invalid token' });
        }
        
        req.user = user;
        next();
    } catch (error) {
        return res.status(403).json({ error: 'Invalid token' });
    }
};

// Database initialization
async function initializeDatabase() {
    try {
        await pool.query(`
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                email VARCHAR(255) UNIQUE NOT NULL,
                password_hash VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                is_active BOOLEAN DEFAULT true
            )
        `);

        await pool.query(`
            CREATE TABLE IF NOT EXISTS packages (
                id SERIAL PRIMARY KEY,
                name VARCHAR(100) NOT NULL,
                version VARCHAR(50) NOT NULL,
                description TEXT,
                author_id INTEGER REFERENCES users(id),
                keywords TEXT[],
                license VARCHAR(50),
                homepage VARCHAR(255),
                repository_url VARCHAR(255),
                dependencies JSONB DEFAULT '{}',
                dev_dependencies JSONB DEFAULT '{}',
                tarball_path VARCHAR(255),
                tarball_size INTEGER,
                tarball_sha256 VARCHAR(64),
                published_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                download_count INTEGER DEFAULT 0,
                UNIQUE(name, version)
            )
        `);

        await pool.query(`
            CREATE TABLE IF NOT EXISTS package_maintainers (
                package_name VARCHAR(100) NOT NULL,
                user_id INTEGER REFERENCES users(id),
                role VARCHAR(20) DEFAULT 'maintainer',
                added_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY (package_name, user_id)
            )
        `);

        await pool.query(`
            CREATE TABLE IF NOT EXISTS download_stats (
                id SERIAL PRIMARY KEY,
                package_name VARCHAR(100) NOT NULL,
                package_version VARCHAR(50) NOT NULL,
                downloaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                ip_address INET,
                user_agent TEXT
            )
        `);

        console.log('Database initialized successfully');
    } catch (error) {
        console.error('Database initialization failed:', error);
        process.exit(1);
    }
}

// User management functions
async function createUser(username, email, password) {
    const passwordHash = await bcrypt.hash(password, 10);
    
    const result = await pool.query(
        'INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id, username, email',
        [username, email, passwordHash]
    );
    
    return result.rows[0];
}

async function getUserById(id) {
    const result = await pool.query('SELECT * FROM users WHERE id = $1 AND is_active = true', [id]);
    return result.rows[0];
}

async function getUserByUsername(username) {
    const result = await pool.query('SELECT * FROM users WHERE username = $1 AND is_active = true', [username]);
    return result.rows[0];
}

async function validateUser(username, password) {
    const user = await getUserByUsername(username);
    if (!user) return null;
    
    const isValid = await bcrypt.compare(password, user.password_hash);
    return isValid ? user : null;
}

// Package management functions
async function createPackage(packageData, authorId) {
    const {
        name, version, description, keywords, license, homepage,
        repository_url, dependencies, dev_dependencies,
        tarball_path, tarball_size, tarball_sha256
    } = packageData;

    const result = await pool.query(`
        INSERT INTO packages (
            name, version, description, author_id, keywords, license,
            homepage, repository_url, dependencies, dev_dependencies,
            tarball_path, tarball_size, tarball_sha256
        ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)
        RETURNING *
    `, [
        name, version, description, authorId, keywords, license,
        homepage, repository_url, JSON.stringify(dependencies),
        JSON.stringify(dev_dependencies), tarball_path, tarball_size, tarball_sha256
    ]);

    return result.rows[0];
}

async function getPackage(name, version = null) {
    let query, params;
    
    if (version) {
        query = 'SELECT * FROM packages WHERE name = $1 AND version = $2';
        params = [name, version];
    } else {
        query = `
            SELECT * FROM packages 
            WHERE name = $1 
            ORDER BY published_at DESC 
            LIMIT 1
        `;
        params = [name];
    }
    
    const result = await pool.query(query, params);
    return result.rows[0];
}

// API Routes
app.post('/api/users/register', async (req, res) => {
    try {
        const { username, email, password } = req.body;
        
        if (!username || !email || !password) {
            return res.status(400).json({ error: 'Username, email, and password are required' });
        }
        
        const user = await createUser(username, email, password);
        const token = jwt.sign({ userId: user.id }, JWT_SECRET, { expiresIn: '30d' });
        
        res.status(201).json({
            message: 'User created successfully',
            user: { id: user.id, username: user.username, email: user.email },
            token
        });
    } catch (error) {
        console.error('Registration error:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

app.post('/api/users/login', async (req, res) => {
    try {
        const { username, password } = req.body;
        
        const user = await validateUser(username, password);
        if (!user) {
            return res.status(401).json({ error: 'Invalid credentials' });
        }
        
        const token = jwt.sign({ userId: user.id }, JWT_SECRET, { expiresIn: '30d' });
        
        res.json({
            message: 'Login successful',
            user: { id: user.id, username: user.username, email: user.email },
            token
        });
    } catch (error) {
        console.error('Login error:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

app.get('/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Start server
async function startServer() {
    try {
        await fs.mkdir(PACKAGES_DIR, { recursive: true });
        await initializeDatabase();
        
        app.listen(PORT, () => {
            console.log(`Rubolt Package Registry running on port ${PORT}`);
        });
    } catch (error) {
        console.error('Failed to start server:', error);
        process.exit(1);
    }
}

if (require.main === module) {
    startServer();
}

module.exports = app;